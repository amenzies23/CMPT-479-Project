#pragma once

#include "../core/contracts.h"
#include <memory>

namespace apr_system {

/**
 * @brief concrete implementation of patch validator component
 *
 * validates patch candidates by applying them and running tests.
 */
class Validator : public IValidator {
public:
  Validator() = default;
  ~Validator() override = default;

  /**
   * @brief validate patch candidates by applying them and running tests
   * @param prioritized_patches list of prioritized patches to validate
   * @param repo_metadata repository metadata for build/test configuration
   * @param top_k number of top patches to validate
   * @return vector of validation results
   */
  std::vector<ValidationResult>
  validatePatches(const std::vector<PrioritizedPatch> &prioritized_patches,
                  const RepositoryMetadata &repo_metadata,
                  int top_k = 10) override;

private:
  /**
   * @brief apply a patch to the source code
   * @param patch_id reference to the patch to apply
   * @param repo_path path to the repository
   * @return true if patch was applied successfully
   */
  bool applyPatch(const std::string &patch_id,
                  const std::string &repo_path) const;

  /**
   * @brief build the project after applying a patch
   * @param repo_path path to the repository
   * @param build_script build script to run
   * @return build result with success status and output
   */
  std::pair<bool, std::string>
  buildProject(const std::string &repo_path,
               const std::string &build_script) const;

  /**
   * @brief run tests after building
   * @param repo_path path to the repository
   * @param test_script test script to run
   * @return test result with pass/fail status and output
   */
  std::pair<bool, std::string> runTests(const std::string &repo_path,
                                        const std::string &test_script) const;
};

} // namespace apr_system
