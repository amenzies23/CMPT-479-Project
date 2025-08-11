#include "utils.h"
#include <fstream>

namespace apr_system { 

void dumpSuspiciousNodes(const std::vector<const ASTNode*>& targets) {
  std::ofstream out("SuspiciousNodes.txt");
  if (!out) return;
  for (auto *n : targets) {
    out
      << "node_id: " << n->node_id
      << ", type: " << n->node_type
      << ", file: " << n->file_path
      << ", range: [" << n->start_line << "," << n->start_column
      << "] - [" << n->end_line   << "," << n->end_column
      << "]\n";
    out << "Source_code: " << n->source_text << "\n";
    out << "Sus_score: " << n->suspiciousness_score << "\n";

    out << "  genealogy_context: {";
    for (auto &kv : n->genealogy_context.type_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n";

    out << "  variable_context: {";
    for (auto &kv : n->variable_context.var_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n";

    out << "  dependency_context: {";
    for (auto &kv : n->dependency_context.slice_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n\n";
  }
}

void dumpPatchCandidates(const std::vector<PatchCandidate>& patches) {
  std::ofstream out("Patch_Candidates.txt");
  if (!out) return;
  for (auto &p : patches) {
    out << "patch_id: " << p.patch_id << "\n"
        << "target_node_id: " << p.target_node_id << "\n"
        << "file_path: " << p.file_path << "\n"
        << "lines: [" << p.start_line << "-" << p.end_line << "]\n"
        << "original_code: " << p.original_code << "\n"
        << "modified_code: " << p.modified_code << "\n"
        << "diff:\n" << p.diff << "\n"
        << "mutation_category: " << p.mutation_type.mutation_category << "\n"
        << "mutation_target_node: " << p.mutation_type.target_node << "\n"
        << "mutation_source_node: " << p.mutation_type.source_node << "\n"
        << "affected_tests: ";
    for (auto &t : p.affected_tests) out << t << "; ";
    out << "\n"
        << "suspiciousness_score: " << p.suspiciousness_score << "\n"
        << "similarity_score: " << p.similarity_score << "\n"
        << "priority_score: " << p.priority_score << "\n\n";
  }
}

void dumpFixIngredients(const std::vector<const ASTNode*>& ingredients) {
  std::ofstream out("fixIngredients.txt");
  if (!out) return;
  for (auto *n : ingredients) {
    out << "node_id: " << n->node_id
        << ", type: " << n->node_type
        << ", file: " << n->file_path
        << ", range: [" << n->start_line << "," << n->start_column
        << "] - [" << n->end_line << "," << n->end_column
        << "]\n";
    out << "Source_code: " << n->source_text << "\n";
    out << "Sus_score: " << n->suspiciousness_score << "\n";

    out << "  genealogy_context: {";
    for (auto &kv : n->genealogy_context.type_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n";

    out << "  variable_context: {";
    for (auto &kv : n->variable_context.var_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n";

    out << "  dependency_context: {";
    for (auto &kv : n->dependency_context.slice_counts)
      out << kv.first << ":" << kv.second << ", ";
    out << "}\n\n";
  }
}

} 