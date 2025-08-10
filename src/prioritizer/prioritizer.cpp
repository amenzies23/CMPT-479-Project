#include "prioritizer.h"
#include "../core/logger.h"


namespace apr_system {

std::vector<PatchCandidate> Prioritizer::prioritizePatches(
    const std::vector<PatchCandidate>& patch_candidates,
    const std::string& mutation_freq_json
) {
    LOG_COMPONENT_INFO("prioritizer", "this is a stub implementation");
    LOG_COMPONENT_INFO("prioritizer", "input: {} patch candidates", patch_candidates.size());

    std::unordered_map<std::string, std::vector<FreqEntry>> freqMap = parseFrequencyFile(mutation_freq_json);

    std::vector<PatchCandidate> prioritized_patches;
    LOG_COMPONENT_INFO("prioritizer", "computing priority scores...");

    // create mock prioritized patches
    for (size_t i = 0; i < patch_candidates.size(); ++i) {
        PatchCandidate candidate = patch_candidates[i];
        const double score = computePriorityScore(candidate, freqMap);
        candidate.priority_score = score;
        // Cuts out any patches with a score of 0
        if (score > 0.0) {
            prioritized_patches.push_back(std::move(candidate));
        }
    }
    
    std::sort(prioritized_patches.begin(), prioritized_patches.end(), 
            [](const PatchCandidate& a, const PatchCandidate& b) {
                    return a.priority_score > b.priority_score;
    });
    // printPrioritizedPatches(prioritized_patches);

    dumpPrioritizedPatchesToFile(prioritized_patches);

    LOG_COMPONENT_INFO("prioritizer", "returning {} mock prioritized patches", prioritized_patches.size());
    return prioritized_patches;
}

/**
 * Note / question for devs (will remove in a later commit):
 * Currently following the CapGen method, this computes the priority score based on the suspiciousness score, similarity score, and historical frequencies.
 * We implemented logic for the suspiciousness and similarity ourselves, and I trust the way we did this. However, we were not
 * able to build up real historical patch data from C++ patches. So the values inside `freq.json` are all mocked values.
 * They follow the numbers in the CapGen paper, but some of the node types that CapGens historical data had do not match types from tree-sitter.
 * I did my best to match similar node types, but regardless this data should not be reliable for our purposes.
 * 
 * I left this using the historical frequencies for now, but we may decide to scrap it all together and just build up the priority from
 * our suspiciousness score and similarity. Its definitely something id love to have working properly with real data in the future which is why I left it,
 * but in the scope of this project and the time remaining we may need to compromise.
 */
double Prioritizer::computePriorityScore(
    const PatchCandidate& patch, 
    std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap 
) const {
    double similarity = patch.similarity_score;
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
    // return similarity * suspiciousness_score;
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
