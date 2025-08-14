#include "mutator/context.h"
#include <tree_sitter/api.h>
#include <set>
#include <vector>
namespace apr_system {

    // These two helper functions essentially recreate the pseudocode in the CapGen paper to build up the genealogy context
    TypeCountMap extractAncestorTypes(TSNode node) {
        TypeCountMap counts;
        while(!ts_node_is_null(node) && std::string(ts_node_type(node)) != "method_definition"){
            node = ts_node_parent(node);
            if(ts_node_is_null(node)){
                break;
            }
            std::string type = ts_node_type(node);
            if(type != "block"){
                counts[type]++;
            }
        }
        return counts;
    }

    TypeCountMap extractSiblingTypes(TSNode node){
        TypeCountMap counts;
        // Need to find the nearest block parent, then iterate through the children
        TSNode parent = ts_node_parent(node);
        while(!ts_node_is_null(parent) && std::string(ts_node_type(parent)) != "block"){
            parent = ts_node_parent(parent);
        }

        if(ts_node_is_null(parent)){
            return counts;
        }

        uint32_t childCount = ts_node_named_child_count(parent);
        for(uint32_t i = 0; i < childCount; ++i){
            TSNode child = ts_node_named_child(parent, i);
            counts[ts_node_type(child)]++;
        }

        return counts;
    }

    GenealogyContext extractGenealogyContext(TSNode node) {
        GenealogyContext context;
        auto ancestors = extractAncestorTypes(node);
        auto siblings = extractSiblingTypes(node);
        // merge counts
        context.type_counts = std::move(ancestors);
        for (auto &kv : siblings) {
            context.type_counts[kv.first] += kv.second;
        }
        return context;
    }

    /**
     * Helper function to extract a set of variables accessed within a passed in node
     * This builds up a set of strings consisting of the node type + name to determine which nodes we have seen
     * Returns a VariableContext with a map consisting of the type and name of the variables in this nodes scope
     */
    VariableContext extractVariableContext(TSNode node, const std::string &source_content){
        VariableContext context;
        std::set<std::string> seen_keys; // Building up a set of Node type+name to ensure its only counted once
        std::vector<TSNode> stack {node}; 

        while(!stack.empty()){
            TSNode current =  stack.back();
            stack.pop_back();

            if(ts_node_is_named(current)){
                std::string node_type = ts_node_type(current);
                if(node_type == "identifier" || node_type=="field_identifier"){
                    int beginning = ts_node_start_byte(current);
                    int end = ts_node_end_byte(current);
                    std::string name = source_content.substr(beginning, end - beginning);
                    std::string key = node_type + "#" + name; // Building up a key of a stringed node type and name. Easier to manage this way
                    if(seen_keys.insert(key).second){
                        context.var_counts[key] = 1;
                    }
                }
            }

            uint32_t childCount = ts_node_named_child_count(current);
            for(uint32_t i = 0; i < childCount; ++i){
                stack.push_back(ts_node_named_child(current, i));
            }
        }
        return context;
    }

    // Helper function to determine if a given AST node is a variable definition node (to be used in slicing functions)
    static bool is_definition_node(TSNode current, const std::string &name, uint32_t cutoff, const std::string &src) {
        const char *type = ts_node_type(current);

        // Each of these checks for a type of declaration / assignment, and returns true if it is the same name as the variable we are looking at

        // Local declaration
        if (strcmp(type, "init_declarator") == 0) {
            TSNode id = ts_node_named_child(current, 0);
            if (!ts_node_is_null(id) && ts_node_is_named(id) && strcmp(ts_node_type(id), "identifier") == 0) {
                uint32_t start = ts_node_start_byte(id);
                if (start <= cutoff && src.substr(start, ts_node_end_byte(id) - start) == name) {
                    return true;
                }
            }
        }
        // Constructor initializer
        else if (strcmp(type, "field_initializer") == 0) {
            TSNode field_id = ts_node_named_child(current, 0);
            if (!ts_node_is_null(field_id) && ts_node_is_named(field_id) 
                    && strcmp(ts_node_type(field_id), "field_identifier") == 0) {
                TSNode id = ts_node_named_child(field_id, 0);
                if (!ts_node_is_null(id) && ts_node_is_named(id) && strcmp(ts_node_type(id), "identifier") == 0) {
                    uint32_t start = ts_node_start_byte(id);
                    if (start <= cutoff && src.substr(start, ts_node_end_byte(id) - start) == name) {
                        return true;
                    }
                }
            }
        }
        // Assignment
        else if (strcmp(type, "assignment_expression") == 0) {
            TSNode lhs = ts_node_named_child(current, 0);
            if (!ts_node_is_null(lhs) && ts_node_is_named(lhs)
                && strcmp(ts_node_type(lhs), "identifier") == 0) {
                uint32_t start = ts_node_start_byte(lhs);
                if (start <= cutoff && src.substr(start, ts_node_end_byte(lhs) - start) == name) {
                    return true;
                }
            }
        }
        return false;
    }

    // Helper function for once we find a definition node, walk to the parent and record the types of children as context
    static void record_definition_context(TSNode node, TSNode root, DependencyContext &ctx) {
        TSNode stmt = node;
        while (!ts_node_is_null(stmt)) {
            const char *type = ts_node_type(stmt);
            if (strcmp(type, "declaration") == 0
                || strstr(type, "statement")
                || strstr(type, "expression")
                || strcmp(type, "field_initializer_list") == 0) {
                break;
            }
            stmt = ts_node_parent(stmt);
        }
        if (ts_node_is_null(stmt)) return;

        // Counting only the leaf nodes
        uint32_t childCount = ts_node_named_child_count(stmt);
        for (uint32_t i = 0; i < childCount; ++i) {
            TSNode child = ts_node_named_child(stmt, i);
            if (!ts_node_is_named(child)) continue;
            const char *type = ts_node_type(child);
            if ( strcmp(type, "identifier") == 0
                || strcmp(type, "primitive_type") == 0
                || strcmp(type, "init_declarator") == 0
                || strcmp(type, "field_identifier") == 0
            ) {
                ctx.slice_counts[type]++;
            }
        }
    }

    /**
     * Grabs all variables used in the target node, walks the entire AST from the root and finds each definition
     * of the variable before the end of target node. For each definition, record its context.
     */
    DependencyContext backwardSlice(TSNode target, TSNode root, const std::string &source_content) {
        DependencyContext context;

        // Extract the variables within the scope of this node
        auto varCtx = extractVariableContext(target, source_content);
        std::vector<std::string> varNames;
        for (auto &kv : varCtx.var_counts) {
            varNames.push_back(kv.first.substr(kv.first.find('#') + 1)); // Since the var names are stored after '#'
        }

        // Cutoff point at the end of the target
        uint32_t cutoff = ts_node_end_byte(target);

        // Traverse entire AST looking for definitions
        std::vector<TSNode> stack{root};
        while (!stack.empty()) {
            TSNode current = stack.back();
            stack.pop_back();
            if (ts_node_is_null(current)) continue;

            // Test each name
            for (auto &name : varNames) {
                if (is_definition_node(current, name, cutoff, source_content)) {
                    record_definition_context(current, root, context);
                    break;
                }
            }

            // Recurse on the children
            uint32_t childCount = ts_node_named_child_count(current);
            for (uint32_t i = 0; i < childCount; ++i) {
                TSNode child = ts_node_named_child(current, i);
                if (!ts_node_is_null(child)) stack.push_back(child);
            }
        }

        return context;
    }

    /**
     * Similar logic to the backward slice. We take a target node N, collect the set of variables that N defines. 
     * Then scan the AST for every use of these variables that occur *after* N in the source code. 
     * For each use site, we locate its nearest enclosing statement/expression and count the types of the nodes named children.
     */
    DependencyContext forwardSlice(TSNode target, TSNode root, const std::string &source_content){
        DependencyContext context;

        auto variableContext = extractVariableContext(target, source_content);
        std::vector<std::string> variableNames;

        for(auto &kv : variableContext.var_counts){
            auto name = kv.first.substr(kv.first.find('#') + 1);
            variableNames.push_back(name);
        }

        uint32_t target_end = ts_node_end_byte(target);

        std::vector<TSNode> stack{ root };
        while (!stack.empty()) {
            TSNode current = stack.back();
            stack.pop_back();

            // we only care about named identifier uses
            if (ts_node_is_named(current)
                && std::string(ts_node_type(current)) == "identifier")
            {
                // Extract its name and its byte‐range
                uint32_t beginning = ts_node_start_byte(current);
                uint32_t end = ts_node_end_byte(current);
                std::string name = source_content.substr(beginning, end - beginning);

                // If this identifier matches one of our varNames AND it occurs after the target end, it's a use site
                if (beginning >= target_end
                    && std::find(variableNames.begin(), variableNames.end(), name) != variableNames.end())
                {
                    // Climb up to the nearest statement/expression
                    TSNode stmt = ts_node_parent(current);
                    while (!ts_node_is_null(stmt) && !strstr(ts_node_type(stmt), "statement")
                           && !strstr(ts_node_type(stmt), "expression"))
                    {
                        stmt = ts_node_parent(stmt);
                    }

                    // Record the context of each child
                    if (!ts_node_is_null(stmt)) {
                        uint32_t childCount = ts_node_named_child_count(stmt);
                        for (uint32_t i = 0; i < childCount; ++i) {
                            TSNode child = ts_node_named_child(stmt, i);
                            if (ts_node_is_named(child)) {
                                context.slice_counts[ ts_node_type(child) ]++;
                            }
                        }
                    }
                }
            }

            // Push the rest of the children to continue iterating
            uint32_t childCount = ts_node_named_child_count(current);
            for (uint32_t i = 0; i < childCount; ++i) {
                TSNode child = ts_node_named_child(current, i);
                if (!ts_node_is_null(child)){
                    stack.push_back(child);
                }
            }
        }

        return context;
    }

    DependencyContext extractDependencyContext(TSNode target, TSNode root, const std::string &source_content) {
        auto back = backwardSlice(target, root, source_content);
        auto fwd  = forwardSlice(target, root, source_content);
        for (auto &kv : fwd.slice_counts)
            back.slice_counts[kv.first] += kv.second;
        return back;
    }

    double computeGenealogySimilarity(
        const GenealogyContext &source,
        const GenealogyContext &target
    ) {
        const auto &typeCountsSource = source.type_counts;
        const auto &typeCountsTarget = target.type_counts;
        int64_t numerator = 0, denominator = 0;

        for (const auto &kv : typeCountsTarget) {
            const std::string &nodeType = kv.first;
            int countInTarget = kv.second;
            denominator += countInTarget;
            auto it = typeCountsSource.find(nodeType);
            if (it != typeCountsSource.end()) {
                numerator += std::min(countInTarget, it->second);
            }
        }

        if (denominator == 0) return 0.0;
        return double(numerator) / double(denominator);
    }

    double computeVariableSimilarity(
        const VariableContext &source,
        const VariableContext &target
    ) {
        std::set<std::string> varsSource, varsTarget;
        for (const auto &kv : source.var_counts) varsSource.insert(kv.first);
        for (const auto &kv : target.var_counts) varsTarget.insert(kv.first);

        std::vector<std::string> intersection;
        std::vector<std::string> unionSet;
        std::set_intersection(
            varsSource.begin(), varsSource.end(),
            varsTarget.begin(), varsTarget.end(),
            std::back_inserter(intersection)
        );
        std::set_union(
            varsSource.begin(), varsSource.end(),
            varsTarget.begin(), varsTarget.end(),
            std::back_inserter(unionSet)
        );

        if (unionSet.empty()) return 1.0;
        return double(intersection.size()) / double(unionSet.size());
    }

    double computeDependencySimilarity(
        const DependencyContext &source,
        const DependencyContext &target
    ) {
        const auto &sliceCountsSource = source.slice_counts;
        const auto &sliceCountsTarget = target.slice_counts;
        int64_t numerator = 0, denominator = 0;

        for (const auto &kv : sliceCountsTarget) {
            const std::string &nodeType = kv.first;
            int countInTarget = kv.second;
            denominator += countInTarget;
            auto it = sliceCountsSource.find(nodeType);
            if (it != sliceCountsSource.end()) {
                numerator += std::min(countInTarget, it->second);
            }
        }

        if (denominator == 0) return 1.0;
        return double(numerator) / double(denominator);
    }

    // Similarity formulas from the capgen paper based on which operation type we are using, they invoke the above 3 formulas
    double computeReplacementSimilarity(
        const GenealogyContext &sourceGenealogy,
        const GenealogyContext &targetGenealogy,
        const DependencyContext &sourceDependency,
        const DependencyContext &targetDependency,
        const VariableContext &sourceVariable,
        const VariableContext &targetVariable
    ) {
        // Simi_R = f_gen(source, target) * f_dep(source, target) * d_var(source, target)
        double gSim = computeGenealogySimilarity(sourceGenealogy, targetGenealogy);
        double dSim = computeDependencySimilarity(sourceDependency, targetDependency);
        double vSim = computeVariableSimilarity(sourceVariable, targetVariable);
        return gSim * dSim * vSim;
    }

    double computeInsertionSimilarity(
        const GenealogyContext &sourceGenealogy,
        const GenealogyContext &targetGenealogy,
        const DependencyContext &sourceDependency,
        const DependencyContext &targetDependency
    ) {
        // Simi_I = f_gen(source, target) * f_dep(source, target)
        double gSim = computeGenealogySimilarity(sourceGenealogy, targetGenealogy);
        double dSim = computeDependencySimilarity(sourceDependency, targetDependency);
        return gSim * dSim;
    }

    double computeDeletionSimilarity(
        const GenealogyContext &otherGenealogy,
        const GenealogyContext &targetGenealogy,
        const DependencyContext &otherDependency,
        const DependencyContext &targetDependency
    ) {
        // Simi_D = (1 − f_gen(other, target)) * (1 − f_dep(other, target))
        double gSimOther = computeGenealogySimilarity(otherGenealogy, targetGenealogy);
        double dSimOther = computeDependencySimilarity(otherDependency, targetDependency);
        // Guard against when the target and other are the same node. This would normally reutrn (1.0-1.0) * (1.0-1.0) = 0
        if (gSimOther==1.0 && dSimOther==1.0) return 1.0;
        return (1.0 - gSimOther) * (1.0 - dSimOther);
    }

}