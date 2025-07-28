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

        int n = ts_node_named_child_count(parent);
        for(int i = 0; i < n; ++i){
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
                if(node_type == "identifier" || node_type=="feld_identifier"){
                    int beginning = ts_node_start_byte(current);
                    int end = ts_node_end_byte(current);
                    std::string name = source_content.substr(beginning, end - beginning);
                    std::string key = node_type + "#" + name; // Building up a key of a stringed node type and name. Easier to manage this way
                    if(seen_keys.insert(key).second){
                        context.var_counts[key] = 1;
                    }
                }
            }

            int childCount = ts_node_named_child_count(current);
            for(int i = 0; i < childCount; ++i){
                stack.push_back(ts_node_named_child(current, i));
            }
        }
        return context;
    }
    /**
     * Given a target AST node N, we collect the set of all variables that N uses (invoking the existing extractVariable function)
     * Then we scan the entire AST from the root to find every definition of each variable in this set that occurs *before*s N in the source.
     * For each definition statement, we count the types of its anmed children (statement / expression nodes)
     */
    DependencyContext backwardSlice(TSNode target, TSNode root, const std::string &source_content){
        DependencyContext context;
        // First grab the variables in the scope of the target node
        auto variableContext = extractVariableContext(target, source_content);
        std::vector<std::string> varNames;
        for (auto &kv : variableContext.var_counts) {
            // Variable context is returned as a key where the name comes after the '#'
            auto name = kv.first.substr(kv.first.find('#') + 1);
            varNames.push_back(name);
        }

        uint32_t target_position = ts_node_start_byte(target);

        for(auto &name : varNames){
            std::vector<TSNode> stack{root};
            while(!stack.empty()){
                TSNode current = stack.back();
                stack.pop_back();
                std::string type = ts_node_type(current);
                bool isDefinition = false;

                if(type == "init_declarator" || type == "declaration" || type == "assignment_expression"){
                    uint32_t childCount = ts_node_named_child_count(current);
                    for(uint32_t i = 0; i < childCount; ++i){
                        TSNode child = ts_node_named_child(current, i);
                        if(ts_node_is_named(child) && std::string(ts_node_type(child)) == "identifier"){
                            uint32_t beginning = ts_node_start_byte(child);
                            uint32_t end = ts_node_end_byte(child);
                            if(end <= target_position && source_content.substr(beginning, end - beginning) == name){
                                isDefinition = true;
                                break;
                            }
                        }
                    }
                }
                // If this is a definition node, count all the named children 
                if(isDefinition){
                    uint32_t count = ts_node_named_child_count(current);
                    for(uint32_t i = 0; i < count; ++i){
                        TSNode child = ts_node_named_child(current, i);
                        std::string type = ts_node_type(child);
                        context.slice_counts[type]++;
                    }
                }

                uint32_t allChildren = ts_node_named_child_count(current);
                for(uint32_t i = 0; i < allChildren; ++i){
                    stack.push_back(ts_node_named_child(current, i));
                }
            }
        }
        return context;
    }

    /**
     * Similar logic to the backward slice. We take a target node N, collect the set of variables that N defines. 
     * Then scan the entire AST for every use of these variables that occur *after* N in the source code. 
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
        uint32_t tpos = ts_node_end_byte(target);

        for (auto &name : variableNames) {
            std::vector<TSNode> stack{root};
            while (!stack.empty()) {
                TSNode current = stack.back(); 
                stack.pop_back();

                if (ts_node_is_named(current) &&
                    std::string(ts_node_type(current)) == "identifier") {
                    uint32_t beginning = ts_node_start_byte(current);
                    uint32_t end = ts_node_end_byte(current);
                    if (beginning >= tpos && source_content.substr(beginning, end - beginning) == name) {
                        // Find enclosing statements / expressions
                        TSNode stmt = ts_node_parent(current);
                        while (!ts_node_is_null(stmt) && !strstr(ts_node_type(stmt), "statement") && !strstr(ts_node_type(stmt), "expression")) {
                            stmt = ts_node_parent(stmt);
                        }
                        if (!ts_node_is_null(stmt)) {
                            uint32_t c = ts_node_named_child_count(stmt);
                            for (uint32_t i = 0; i < c; ++i) {
                                TSNode ch = ts_node_named_child(stmt, i);
                                context.slice_counts[ts_node_type(ch)]++;
                            }
                        }
                    }
                }

                uint32_t all = ts_node_named_child_count(current);
                for (uint32_t i = 0; i < all; ++i) {
                    stack.push_back(ts_node_named_child(current, i));
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

}