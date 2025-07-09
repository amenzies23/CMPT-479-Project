#include "parser.h"
#include "../core/logger.h"

namespace apr_system {

std::vector<ASTNode> Parser::parseAST(
    const std::vector<SuspiciousLocation>& suspicious_locations,
    const std::vector<std::string>& source_files
) {
    LOG_COMPONENT_INFO("parser", "this is a stub implementation");
    LOG_COMPONENT_INFO("parser", "input: {} suspicious locations, {} source files",
                       suspicious_locations.size(), source_files.size());

    /*
     * TODO: implement AST parsing here
     *
     * input contracts:
     * - suspicious_locations: locations identified by SBFL with file_path and line_number
     * - source_files: paths to source files that should be parsed
     *
     * output contract:
     * - return ASTNode objects with valid node_type, source_text, and location info
     * - each node should represent a syntactic element that can be mutated
     * - common node types: "expression", "statement", "function_call", etc.
     */

    // mock data for testing data flow
    std::vector<ASTNode> mock_nodes;

    // create mock AST nodes for each suspicious location
    for (size_t i = 0; i < suspicious_locations.size(); ++i) {
        const auto& location = suspicious_locations[i];

        ASTNode node{
            .node_id = "node_" + std::to_string(i),
            .node_type = i % 2 == 0 ? "expression_statement" : "binary_expression",
            .start_line = location.line_number,
            .end_line = location.line_number,
            .start_column = 1,
            .end_column = 50,
            .file_path = location.file_path,
            .source_text = "[STUB] mock source code at line " + std::to_string(location.line_number),
            .child_node_ids = {}
        };

        mock_nodes.push_back(node);
    }

    LOG_COMPONENT_INFO("parser", "stub returning {} mock AST nodes", mock_nodes.size());
    return mock_nodes;
}

} // namespace apr_system
