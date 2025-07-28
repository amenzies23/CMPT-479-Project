#include "prioritizer.h"
#include "../core/logger.h"



namespace apr_system {

//
// Helper functions to print vectors
//
void printFreqMap(const std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap) {
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
                  << ", Ref: " << patch.patch_id_ref
                  << ", Features: ";
        for (const auto& feat : patch.features) {
            std::cout << feat << "; ";
        }
        std::cout << "Reasoning: " << patch.reasoning << std::endl;
    }
}

std::vector<PrioritizedPatch> Prioritizer::prioritizePatches(
    const std::vector<PatchCandidate>& patch_candidates,
    const std::string& mutation_freq_json
) {
    LOG_COMPONENT_INFO("prioritizer", "this is a stub implementation");
    LOG_COMPONENT_INFO("prioritizer", "input: {} patch candidates", patch_candidates.size());

    /*
     * TODO: implement patch prioritization here
     *
     * input contracts:
     * - patch_candidates: generated patch candidates from mutator
     * - test_results: test execution results for feature extraction
     *
     * output contract:
     * - return PrioritizedPatch objects sorted by priority_score (1.0 = highest priority)
     * - each patch should include features used for ranking and reasoning
     */

    // mock data for testing data flow -> remove it when implementing


    std::unordered_map<std::string, std::vector<FreqEntry>> freqMap = parseFrequencyFile(mutation_freq_json);

    std::vector<PrioritizedPatch> mock_prioritized;

    // create mock prioritized patches
    for (size_t i = 0; i < patch_candidates.size(); ++i) {
        const PatchCandidate candidate = patch_candidates[i];
        LOG_COMPONENT_INFO("prioritizer", "computing priority scores...");
        const double score = computePriorityScore(candidate, freqMap);

        PrioritizedPatch prioritized{
            .patch_id = "prioritized_" + std::to_string(i),
            .priority_score = score, // mock decreasing priority
            .patch_id_ref = candidate.patch_id,
            .features = {
                "mutation_type:" + candidate.mutation_type.mutation_category,
                "line_count:" + std::to_string(candidate.end_line - candidate.start_line + 1),
                "complexity:low"
            },
            .reasoning = "[STUB] mock prioritization based on mutation type and location"
        };

        mock_prioritized.push_back(prioritized);
    }

    // printPrioritizedPatches(mock_prioritized);

    LOG_COMPONENT_INFO("prioritizer", "stub returning {} mock prioritized patches", mock_prioritized.size());
    return mock_prioritized;
}

double Prioritizer::computePriorityScore(
    const PatchCandidate& patch, 
    std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap 
) const {
    double similarity = patch.similarity;
    double suspiciousness_score = patch.suspiciousness_score;
    double freqScore = 0.0;

    std::string category = patch.mutation_type.mutation_category;
    std::string target = patch.mutation_type.target_node;
    std::string source = patch.mutation_type.source_node;

    std::vector<FreqEntry> entries = freqMap[category];

    for (const auto& entry : entries) {
        if (category == "Replacement") {
            if (entry.target_node == target) {
                freqScore = entry.freq;
            }
        } else {
            if (entry.target_node == target && entry.source_node == source) {
                freqScore = entry.freq;
            }
        }
    }
    return similarity * suspiciousness_score * freqScore;
}




// helper function to add mutation frequencies to mapping
void addToFreqMap(
    const nlohmann::json& data, 
    std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap,
    std::string mutation
) {
    std::vector<FreqEntry> freqEntries;

    if (data.contains(mutation)) {
        const auto& data_array = data[mutation];
        for (const auto& item : data_array) {
            FreqEntry freqEntry;
            std::string target = item.value("target", "unknown");
            std::string source = item.value("source", "unknown");
            double freq = item.value("freq", 0.0);

            freqEntry.target_node = target;
            freqEntry.source_node = source;
            freqEntry.freq = freq;
            freqEntries.push_back(freqEntry);
        }

        freqMap[mutation] = freqEntries;
    }
}

std::unordered_map<std::string, std::vector<FreqEntry>> 
Prioritizer::parseFrequencyFile(const std::string& freqFile) const {
    std::unordered_map<std::string, std::vector<FreqEntry>> freqMap;

    LOG_COMPONENT_INFO("prioritizer", "parsing JSON mutation frequencies from: {}", freqFile);

    try {
        std::ifstream file(freqFile);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open output file: " + freqFile);
        }

        nlohmann::json data = nlohmann::json::parse(file);

        addToFreqMap(data, freqMap, "Replacement");
        addToFreqMap(data, freqMap, "Insertion");
        addToFreqMap(data, freqMap, "Deletion");

        // printFreqMap(freqMap);
    } catch (const std::exception& e) {
        LOG_COMPONENT_ERROR("prioritizer", "error parsing JSON frequency file: {}", e.what());
    }

    return freqMap;
}


} // namespace apr_system
