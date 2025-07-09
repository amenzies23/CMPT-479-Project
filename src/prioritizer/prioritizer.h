#pragma once

#include "../core/contracts.h"
#include <memory>

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
   * @return vector of prioritized patches sorted by priority score
   */
  std::vector<PrioritizedPatch>
  prioritizePatches(const std::vector<PatchCandidate> &patch_candidates,
                    const std::vector<TestResult> &test_results) override;

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
   * @param features extracted features
   * @return priority score (higher is better)
   */
  double computePriorityScore(const std::vector<std::string> &features) const;

  /**
   * @brief generate reasoning for the priority score
   * @param features extracted features
   * @param score computed priority score
   * @return reasoning string
   */
  std::string generateReasoning(const std::vector<std::string> &features,
                                double score) const;
};

} // namespace apr_system
