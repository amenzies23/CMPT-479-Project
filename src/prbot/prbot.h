#pragma once

#include "../core/contracts.h"
#include <memory>

namespace apr_system {

/**
 * @brief concrete implementation of PR bot component
 *
 * creates pull requests with the best validated patches.
 */
class PRBot : public IPRBot {
public:
  PRBot() = default;
  ~PRBot() override = default;

  /**
   * @brief create a pull request with the best validated patch
   * @param best_patch the best validated patch
   * @param repo_metadata repository metadata
   * @param validation_results all validation results for summary
   * @return PR creation result
   */
  PRResult createPullRequest(
      const ValidationResult &best_patch,
      const RepositoryMetadata &repo_metadata,
      const std::vector<ValidationResult> &validation_results) override;

private:
  /**
   * @brief generate PR title based on the patch
   * @param best_patch the best validated patch
   * @return PR title
   */
  std::string generatePRTitle(const ValidationResult &best_patch) const;

  /**
   * @brief generate PR description with patch summary
   * @param best_patch the best validated patch
   * @param validation_results all validation results
   * @return PR description
   */
  std::string generatePRDescription(
      const ValidationResult &best_patch,
      const std::vector<ValidationResult> &validation_results) const;

  /**
   * @brief create a new branch for the PR
   * @param repo_path repository path
   * @param branch_name branch name
   * @return true if branch creation was successful
   */
  bool createBranch(const std::string &repo_path,
                    const std::string &branch_name) const;

  /**
   * @brief push the branch to remote repository
   * @param repo_path repository path
   * @param branch_name branch name
   * @return true if push was successful
   */
  bool pushBranch(const std::string &repo_path,
                  const std::string &branch_name) const;
};

} // namespace apr_system
