#pragma once

#include "../core/contracts.h"
#include "../core/logger.h"
#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <functional>

namespace apr_system {

// validation config for two-phase approach
struct ValidationConfig {
  int time_budget_minutes;
  int max_patches_to_validate;
  bool enable_early_exit;
  ValidationConfig() : time_budget_minutes(70), max_patches_to_validate(10), enable_early_exit(true) {}
  ValidationConfig(int budget_minutes, int max_patches, bool early_exit = true)
      : time_budget_minutes(budget_minutes), max_patches_to_validate(max_patches), enable_early_exit(early_exit) {}
};

// timing metrics for PHASE A and PHASE B execution
struct PhaseTiming {
  long long phase_a_time_ms;
  long long phase_b_time_ms;
  long long total_time_ms;
  PhaseTiming() : phase_a_time_ms(0), phase_b_time_ms(0), total_time_ms(0) {}
};

struct ExecResult {
  bool ok{false};
  std::string output;
  int exit_code{-1};
};

struct TestRunResult {
  bool success{false};
  std::string artifact_path;   // set even on failure
  std::string stdout_text;     // captured stdout/stderr
  int exit_code{-1};
};

// validator implements two-phase patch validation:
// PHASE A: run only failing test cases (fast filter)
// PHASE B: run full test suite if PHASE A passes
class Validator : public IValidator {
public:
  Validator() {}
  explicit Validator(const ValidationConfig& config) : config_(config) {}
  virtual ~Validator() {}

  // validate top-k patches within time budget using gtest
  // expects repo_metadata.test_script to contain path to gtest binary
  // gtest flags (--gtest_filter, --gtest_output) are added automatically
  virtual std::vector<ValidationResult>
  validatePatches(const std::vector<PatchCandidate> &prioritized_patches,
                  const RepositoryMetadata &repo_metadata,
                  int top_k = 10);

  // set validation config (time budget, max patches, early exit)
  void setConfig(const ValidationConfig& config) { config_ = config; }

  // get current validation config
  const ValidationConfig& getConfig() const { return config_; }
  
  // get timing metrics from last validation run
  const PhaseTiming& getPhaseTiming() const { return phase_timing_; }

private:
  // PHASE A: validate patch against originally failing tests only
  ValidationResult validateFailingTests(const PatchCandidate& patch,
    const RepositoryMetadata& repo_metadata,
    const std::chrono::high_resolution_clock::time_point& validation_start_time);

  // PHASE B: validate patch against full regression test suite
  ValidationResult validateRegressionTests(const PatchCandidate& patch,
    const RepositoryMetadata& repo_metadata,
    const ValidationResult& phase_a_result,
    const std::chrono::high_resolution_clock::time_point& validation_start_time);

  // patch/file ops
  bool applyPatch(const PatchCandidate& patch, const std::string& repo_path);
  bool restoreOriginalCode(const PatchCandidate& patch, const std::string& repo_path);
  // build/test
  std::pair<bool, std::string> buildProject(const std::string& repo_path,
    const std::string& build_script,
    const std::chrono::high_resolution_clock::time_point& validation_start_time);


  // run gtest with optional filtering for specific test cases and time budget
  TestRunResult runGTests(const std::string& repo_path,
    const std::string& test_binary,
    const std::vector<std::string>& test_filter,
    const std::chrono::high_resolution_clock::time_point& validation_start_time,
    const std::string& phase_name,
    const std::string& patch_id);

  // execute shell command
  ExecResult executeCommand(const std::string& command,
    const std::string& working_dir,
    long long timeout_ms = -1);

  // budget helpers
  bool isTimeBudgetExceeded(const std::chrono::high_resolution_clock::time_point& start_time) const;
  long long getRemainingTimeBudgetMs(const std::chrono::high_resolution_clock::time_point& start_time) const;

  // flow helpers
  void recordTotalValidationTime(const std::chrono::high_resolution_clock::time_point& start_time);
  ValidationResult validatePatchTwoPhase(const PatchCandidate& patch,
                                         const RepositoryMetadata& repo_metadata,
                                         const std::chrono::high_resolution_clock::time_point& validation_start_time);

  template <typename ValidationFunc>
  ValidationResult timedValidation(ValidationFunc&& func, long long& timing_accumulator);

  // file helpers
  bool fileExists(const std::string& path);
  std::optional<std::vector<std::string>> readFileLines(const std::string& path);
  bool writeFileLines(const std::string& path, const std::vector<std::string>& lines);
  std::vector<std::string> splitIntoLines(const std::string& text);
  bool isValidLineRange(const PatchCandidate& patch, size_t file_size);
  std::vector<std::string> applyPatchToLines(const std::vector<std::string>& original_lines,
                                             const std::vector<std::string>& modified_lines,
                                             const PatchCandidate& patch);
  
  // gtest result parsing
  std::pair<int, int> parseGTestResults(const std::string& xml_path);

  ValidationConfig config_;
  mutable PhaseTiming phase_timing_;

  // repo root resolution
  std::string resolveRepoPathForPatch(const PatchCandidate& patch) const;

  // snapshot of original file contents before applying a patch
  // key: absolute file path; value: full file content lines
  std::unordered_map<std::string, std::vector<std::string>> original_file_cache_;
};

} // namespace apr_system
