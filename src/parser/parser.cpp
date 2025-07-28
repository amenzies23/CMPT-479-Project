#include "parser.h"
#include "../core/logger.h"

#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <tree_sitter/api.h>
#include "../mutator/context.h"
#include <functional>
#include <iostream>

extern "C" const TSLanguage *tree_sitter_cpp();

namespace apr_system {

// Helper, read entire file content into a string
std::string read_file(const std::string &filename) {
    std::string full_path = std::string(PROJECT_SOURCE_DIR) + "/" + filename;
    std::ifstream file(full_path);
    
    if (!file) {
        throw std::runtime_error("Cannot open file: " + full_path + " (" + std::strerror(errno) + ")");
    }
    
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Helper, convert line number to byte position in source code
int get_byte_position_for_line(const std::string &source_content, int target_line) {
    int byte_pos = 0;
    int line = 1;
    
    for (size_t i = 0; i < source_content.size(); ++i) {
        if (line == target_line) {
            return byte_pos;
        }
        
        if (source_content[i] == '\n') {
            line++;
        }
        byte_pos++;
    }
    
    // If target line is beyond file end, return last byte
    if (target_line > line) {
        return static_cast<int>(source_content.size()) - 1;
    }
    
    return -1;  // Should not happen
}

// Helper, create an ASTNode from a Tree-sitter syntax tree node with SBFL metadata
ASTNode create_ast_node(TSNode ast_Node, TSNode root_node, const std::string &source_content, 
                       int &unique_node_counter, const std::string& file_path,
                       double suspiciousness_score = 0.0, const std::string& sbfl_reason = "") {
    // Get where this syntax element starts and ends in the file (byte positions)
    uint32_t byte_start_pos = ts_node_start_byte(ast_Node);
    uint32_t byte_end_pos = ts_node_end_byte(ast_Node);
    
    // Get where this syntax element starts and ends (line/column positions)
    TSPoint line_column_start = ts_node_start_point(ast_Node);
    TSPoint line_column_end = ts_node_end_point(ast_Node);
    
    // Extract the actual source code text for this syntax element
    std::string source_code = source_content.substr(
        byte_start_pos, 
        byte_end_pos - byte_start_pos
    );
    
    // Create and populate our custom ASTNode structure
    ASTNode parsed_AST_node;
    parsed_AST_node.node_id = "node_" + std::to_string(unique_node_counter++);
    parsed_AST_node.node_type = ts_node_type(ast_Node); 
    parsed_AST_node.start_line = line_column_start.row + 1;
    parsed_AST_node.end_line = line_column_end.row + 1;
    parsed_AST_node.start_column = line_column_start.column + 1;
    parsed_AST_node.end_column = line_column_end.column + 1;
    parsed_AST_node.file_path = file_path;
    parsed_AST_node.source_text = source_code;
    parsed_AST_node.child_node_ids = {}; // Empty for now
    
    // **NEW**: Incorporate SBFL metadata
    parsed_AST_node.suspiciousness_score = suspiciousness_score;
    parsed_AST_node.sbfl_reason = sbfl_reason;

    parsed_AST_node.genealogy_context = extractGenealogyContext(ast_Node);
    parsed_AST_node.variable_context = extractVariableContext(ast_Node, source_content);
    parsed_AST_node.dependency_context =extractDependencyContext(ast_Node, root_node, source_content);
    
    return parsed_AST_node;
}

// Helper, check if a syntax tree node covers the suspicious byte position
bool correct_pos(TSNode ast_Node, uint32_t sus_byte_pos) {
    uint32_t node_start_byte = ts_node_start_byte(ast_Node);
    uint32_t node_end_byte = ts_node_end_byte(ast_Node);
    return (node_start_byte <= sus_byte_pos && sus_byte_pos <= node_end_byte);
}

// Helper, recursively walk through syntax tree and collect nodes that cover the suspicious position
// void walk_tree(TSNode curr_node, TSNode root_node, const std::string &source_content, uint32_t sus_byte_pos, 
//                std::vector<ASTNode>& node_AST, int &unique_node_counter, const std::string& file_path,
//                double suspiciousness_score, const std::string& sbfl_reason) {
//     // If were looking at a suspicious node, add the score in. If not, we just pass 0.0 and empty string
//     bool is_susp = correct_pos(curr_node, sus_byte_pos);
//     ASTNode parsed_node = create_ast_node(
//         curr_node, root_node, source_content, unique_node_counter, file_path,
//         is_susp ? suspiciousness_score : 0.0,  is_susp ? sbfl_reason : ""
//     );

//     node_AST.push_back(parsed_node);
    
//     // Recursively check all child syntax nodes
//     uint32_t number_of_child_nodes = ts_node_child_count(curr_node);
//     for (uint32_t child_index = 0; child_index < number_of_child_nodes; ++child_index) {
//         TSNode child_syntax_node = ts_node_child(curr_node, child_index);
//         walk_tree(child_syntax_node, root_node, source_content, sus_byte_pos, node_AST, 
//                  unique_node_counter, file_path, suspiciousness_score, sbfl_reason);
//     }
// }

// Helper, parse a single source file into a syntax tree using Tree-sitter
TSTree* parse_file_into_AST(const std::string& file_path, const std::string& source_content) {
    // Create a new Tree-sitter parser
    TSParser *parser = ts_parser_new();
    
    // Tell the parser we're parsing C++ code
    ts_parser_set_language(parser, tree_sitter_cpp());
    
    // Parse the source code into a syntax tree
    TSTree *AST = ts_parser_parse_string(parser, nullptr, 
                                                source_content.c_str(), 
                                                source_content.size());
    
    // Clean up the parser (we only need the tree)
    ts_parser_delete(parser);
    
    if (!AST) {
        LOG_COMPONENT_ERROR("parser", "Failed to parse file: {}", file_path);
        return nullptr;
    }
    
    return AST;
}

// Helper, parse all source files and store the syntax trees and source code content
std::pair<std::unordered_map<std::string, TSTree*>, std::unordered_map<std::string, std::string>>
parse_files_into_AST(const std::vector<std::string>& source_file_paths) {



    // Maps to store: file_path -> AST and file_path -> source_content
    std::unordered_map<std::string, TSTree*> path_to_AST;
    std::unordered_map<std::string, std::string> source_content_path;
    
    for (const auto& curr_file_path : source_file_paths) { // read each cpp from src/
        try {
            // Read the source code from the file
            std::string source_content = read_file(curr_file_path);
            source_content_path[curr_file_path] = source_content;
            
            // Log information about the file
            int total_file_lines = std::count(source_content.begin(), source_content.end(), '\n');
            LOG_COMPONENT_INFO("parser", "File '{}' has {} lines", curr_file_path, total_file_lines);
            
            // Parse the source code into a syntax tree
            TSTree* parsed_AST = parse_file_into_AST(curr_file_path, source_content);
            if (parsed_AST) {
                path_to_AST[curr_file_path] = parsed_AST;
            }
            
        } catch (const std::exception& file_reading_error) {
            LOG_COMPONENT_ERROR("parser", "Exception reading file {}: {}", curr_file_path, file_reading_error.what());
            continue;
        }
    }
    
    return {path_to_AST, source_content_path};
}

// Helper, process a single suspicious location and extract relevant syntax nodes
// void process_suspicious_location(const SuspiciousLocation& sus_loc,
//                                        const std::unordered_map<std::string, TSTree*>& path_to_AST,
//                                        const std::unordered_map<std::string, std::string>& source_content_path,
//                                        std::vector<ASTNode>& node_AST,
//                                        int& unique_node_counter) {

//     // Find the syntax tree for this suspicious file
//     auto lookup_AST = path_to_AST.find(sus_loc.file_path);
//     if (lookup_AST == path_to_AST.end()) {
//         LOG_COMPONENT_WARN("parser", "File not parsed: {}", sus_loc.file_path);
//         return;
//     }
    
//     // Get the source code content and syntax tree for this file
//     const std::string& source_code_content = source_content_path.at(sus_loc.file_path);
//     TSTree* parsed_AST = lookup_AST->second;
    
//     LOG_COMPONENT_INFO("parser", "Processing suspicious location: line {} in file {} (score: {:.3f})", 
//                       sus_loc.line_number, sus_loc.file_path, sus_loc.suspiciousness_score);
    
//     // Convert the suspicious line number to a byte position in the file
//     int sus_byte_pos = get_byte_position_for_line(source_code_content, sus_loc.line_number);
//     if (sus_byte_pos == -1) {
//         LOG_COMPONENT_WARN("parser", "Suspicious line {} not found in file {}",
//                           sus_loc.line_number, sus_loc.file_path);
//         return;
//     }
    
//     // Get the root of the syntax tree and collect all nodes covering the suspicious position
//     TSNode root_syntax_node = ts_tree_root_node(parsed_AST);
//     walk_tree(root_syntax_node, root_syntax_node, source_code_content, sus_byte_pos, node_AST, 
//              unique_node_counter, sus_loc.file_path, 
//              sus_loc.suspiciousness_score, sus_loc.reason);
// }

// Helper, clean up Tree-sitter syntax tree memory
void cleanup(const std::unordered_map<std::string, TSTree*>& path_to_AST) {
    for (const auto& file_and_tree_pair : path_to_AST) {
        ts_tree_delete(file_and_tree_pair.second);
    }
}

// Parse source code and extract syntax nodes for suspicious bug locations
std::vector<ASTNode> Parser::parseAST(
    const std::vector<SuspiciousLocation>& sus_loc,
    const std::vector<std::string>& source_file_paths
) {
    LOG_COMPONENT_INFO("parser",
        "Starting AST parse: {} suspicious locations, {} source files",
        sus_loc.size(), source_file_paths.size());

    auto [path_to_AST, source_content_path] = parse_files_into_AST(source_file_paths);

    // Group SBFL locations by file
    std::unordered_map<std::string,std::vector<SuspiciousLocation>> sus_by_file;
    for (auto &sl : sus_loc) {
        sus_by_file[sl.file_path].push_back(sl);
        std::cout << sl.suspiciousness_score;
    }

    std::vector<ASTNode> nodes_AST;
    int unique_node_counter = 0;

    // For each file, we will walk the full AST once using a helper function
    for (auto &p : path_to_AST) {
        const auto &file_path = p.first;
        TSTree *tree = p.second;
        const auto &source = source_content_path[file_path];

        // Building parallel vectors of byte‐pos, score & reason
        std::vector<uint32_t> sus_bytes;
        std::vector<double> sus_scores;
        std::vector<std::string> sus_reasons;
        for (auto &sl : sus_by_file[file_path]) {
            int byte = get_byte_position_for_line(source, sl.line_number);
            if (byte >= 0) {
                sus_bytes.push_back(static_cast<uint32_t>(byte));
                sus_scores.push_back(sl.suspiciousness_score);
                sus_reasons.push_back(sl.reason);
            }
        }

        // Function to help recursively walk the AST once per file. 
        std::function<void(TSNode,TSNode)> walk = [&](TSNode node, TSNode root) {
            // Determine if this node covers any of our sus_bytes
            double score = 0.0;
            std::string reason;
            uint32_t start = ts_node_start_byte(node), end  = ts_node_end_byte(node);
            for (size_t i = 0; i < sus_bytes.size(); ++i) {
                if (sus_bytes[i] >= start && sus_bytes[i] <= end) {
                    score  = sus_scores[i];
                    reason = sus_reasons[i];
                    break;
                }
            }

            nodes_AST.push_back(
                create_ast_node(node, root, source, unique_node_counter, file_path, score, reason)
            );

            uint32_t count = ts_node_child_count(node);
            for (uint32_t i = 0; i < count; ++i) {
                walk(ts_node_child(node, i), root);
            }
        };

        TSNode root = ts_tree_root_node(tree);
        walk(root, root);
    }

    cleanup(path_to_AST);
    LOG_COMPONENT_INFO("parser", "Returning {} AST nodes covering suspicious locations",
                    nodes_AST.size());
    return nodes_AST;
}

}