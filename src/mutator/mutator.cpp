#include "mutator.h"
#include "../core/logger.h"
#include <fstream> 

namespace apr_system {

std::vector<PatchCandidate> Mutator::generatePatches(
    const std::vector<ASTNode>& ast_nodes,
    const std::vector<std::string>& source_files
) {
    LOG_COMPONENT_INFO("mutator", "this is a stub implementation");
    LOG_COMPONENT_INFO("mutator", "input: {} AST nodes, {} source files",
                       ast_nodes.size(), source_files.size());

    /*
     * TODO: implement patch generation here
     *
     * input contracts:
     * - ast_nodes: AST nodes from parser representing code that can be mutated
     * - source_files: original source file paths for context
     *
     * output contract:
     * - return PatchCandidate objects with valid patch_id, file_path, line numbers
     * - each patch should have original_code, modified_code, and diff
     * - mutation_type should indicate the type of change applied
     */

    std::vector<const ASTNode*> targets, ingredients;
    for (auto &node : ast_nodes) {
        if (node.suspiciousness_score > 0.0) {
            targets.push_back(&node);
        } else {
            ingredients.push_back(&node);
        }
    }

    // For testing: Using to confirm if the context / node info is being stored properly. 
    // Will remove in a later commit - but useful to check as we test the functionailtiy of the mutator component.
    std::ofstream out("SuspiciousNodes.txt");
    if (out) {
        for (auto *n : targets) {
            out
              << "node_id: "   << n->node_id
              << ", type: "    << n->node_type
              << ", file: "    << n->file_path
              << ", range: ["  << n->start_line << "," << n->start_column
              << "] - ["      << n->end_line   << "," << n->end_column
              << "]\n";
            
            out << "Source_code: " << n->source_text << "\n";
            
            out << "Sus_score: " << n->suspiciousness_score << "\n";

            // genealogy context
            out << "  genealogy_context: {";
            for (auto &kv : n->genealogy_context.type_counts) {
                out << kv.first << ":" << kv.second << ", ";
            }
            out << "}\n";

            // variable context
            out << "  variable_context: {";
            for (auto &kv : n->variable_context.var_counts) {
                out << kv.first << ":" << kv.second << ", ";
            }
            out << "}\n";

            // dependency context
            out << "  dependency_context: {";
            for (auto &kv : n->dependency_context.slice_counts) {
                out << kv.first << ":" << kv.second << ", ";
            }
            out << "}\n\n";
        }
        out.close();
    }

    // mock data for testing data flow - remove when implementing
    std::vector<PatchCandidate> mock_patches;

    // create mock patches for each AST node
    /*
    for (size_t i = 0; i < ast_nodes.size(); ++i) {
        const auto& node = ast_nodes[i];

        PatchCandidate patch{
            .patch_id = "patch_" + std::to_string(i),
            .file_path = node.file_path,
            .start_line = node.start_line,
            .end_line = node.end_line,
            .original_code = node.source_text,
            .modified_code = "[STUB] Modified: " + node.source_text,
            .diff = "@@ -" + std::to_string(node.start_line) + ",1 +" + std::to_string(node.start_line) + ",1 @@\n"
                   + "-" + node.source_text + "\n"
                   + "+[STUB] Modified: " + node.source_text,
            .mutation_type = i % 3 == 0 ? "arithmetic_operator" :
                           i % 3 == 1 ? "logical_operator" : "string_literal",
            .affected_tests = {"test_" + std::to_string(i)}
        };

        mock_patches.push_back(patch);
    }
    */

    // more simple mock patches
    PatchCandidate patch1;
    patch1.patch_id = "patch_0";
    patch1.file_path = "src/example.cpp";
    patch1.start_line = 10;
    patch1.end_line = 10;
    patch1.original_code = "return false;";
    patch1.modified_code = "return true;";
    patch1.diff = "- return false;\n+ return true;";
    patch1.mutation_type = "boolean_replacement";
    patch1.affected_tests.push_back("test_example");
    mock_patches.push_back(patch1);

    PatchCandidate patch2;
    patch2.patch_id = "patch_1";
    patch2.file_path = "src/example.cpp";
    patch2.start_line = 15;
    patch2.end_line = 15;
    patch2.original_code = "x + y";
    patch2.modified_code = "x - y";
    patch2.diff = "- x + y\n+ x - y";
    patch2.mutation_type = "arithmetic_operator";
    patch2.affected_tests.push_back("test_arithmetic");
    mock_patches.push_back(patch2);

    LOG_COMPONENT_INFO("mutator", "stub returning {} mock patch candidates", mock_patches.size());
    return mock_patches;
}

} // namespace apr_system
