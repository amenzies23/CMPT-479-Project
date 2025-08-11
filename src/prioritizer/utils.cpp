#include "utils.h"
#include <iostream>
#include <fstream>

namespace apr_system {

void printFreqMap(
    const std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap) {
    for (const auto& [mutation, entries] : freqMap) {
        std::cout << "Mutation type: " << mutation << std::endl;
        for (const auto& entry : entries) {
            std::cout << "  target: " << entry.target_node
                      << ", source: " << entry.source_node
                      << ", freq: " << entry.freq << std::endl;
        }
    }
}

void printPrioritizedPatches(const std::vector<PrioritizedPatch>& patches) {
    for (const auto& patch : patches) {
        std::cout << "Patch ID: " << patch.patch_id
                  << ", Score: " << patch.priority_score
                  << ", Ref: "   << patch.patch_id_ref
                  << ", Features: ";
        for (const auto& feat : patch.features) {
            std::cout << feat << "; ";
        }
        std::cout << "Reasoning: " << patch.reasoning << std::endl;
    }
}

void dumpPrioritizedPatchesToFile(const std::vector<PatchCandidate>& patches) {
    std::ofstream fout("Prioritized_Patches.txt");
    if (!fout.is_open()) return;
    int count = 1;
    for (const auto &p : patches) {
        fout 
          << "patch #: " << count << "\n"
          << "node_id: " << p.target_node_id << "\n"
          << "similarity_score: "<< p.similarity_score << "\n"
          << "priority_score: " << p.priority_score << "\n"
          << "mutation_type: " << p.mutation_type.mutation_category
          << " (target=" << p.mutation_type.target_node
          << ", source=" << p.mutation_type.source_node << ")\n"
          << "lines: [" << p.start_line << "-" << p.end_line << "]\n"
          << "Original: " << p.original_code << "\n"
          << "Modified: " << p.modified_code << "\n"
          << "Diff:\n" << p.diff << "\n\n";
        ++count;
    }
}

} // namespace apr_system