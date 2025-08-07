#pragma once

#include "../core/contracts.h"
#include <memory>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>


namespace apr_system {

/**
 * @brief concrete implementation of patch prioritizer component
 *
 * prioritizes patch candidates based on various heuristics and features.
 */
class Prioritizer : public IPrioritizer {
public:
  Prioritizer() = default;
  ~Prioritizer() override = default;

  /**
   * @brief prioritize patch candidates based on various heuristics
   * @param patch_candidates list of patch candidates
   * @param test_results test execution results for feature extraction
   * @param mutation_freq_json mutation frequencies
   * @return vector of prioritized patches sorted by priority score
   */
  std::vector<PrioritizedPatch>
  prioritizePatches(const std::vector<PatchCandidate> &patch_candidates,
                    const std::string& mutation_freq_json);

private:
  /**
   * @brief extract features from a patch candidate
   * @param patch patch candidate to analyze
   * @param test_results test results for context
   * @return vector of feature strings
   */
  std::vector<std::string>
  extractFeatures(const PatchCandidate &patch,
                  const std::vector<TestResult> &test_results) const;

  /**
   * @brief compute priority score based on features
   * @param patch patch candidate to compute priority score 
   * @param freqMap frequency mapping from historical data
   * @return priority score (higher is better)
   */
  double 
  computePriorityScore(const PatchCandidate& patch, std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap) const;

  /**
   * @brief generate reasoning for the priority score
   * @param features extracted features
   * @param score computed priority score
   * @return reasoning string
   */
  std::string 
  generateReasoning(const std::vector<std::string> &features,
                                double score) const;

  /**
   * @brief extract frequencies from data file
   * @param freqFile frequency file to parse
   * @return unordered map with mutations as keys and vector of frequency entries as values
   */
  std::unordered_map<std::string, std::vector<FreqEntry>> 
  parseFrequencyFile(const std::string& freqFile) const;
};

} // namespace apr_system
