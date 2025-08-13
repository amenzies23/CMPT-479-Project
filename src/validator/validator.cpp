#include "validator.h"
#include "../core/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <functional>
#include <numeric>
#include <array>
#include <optional>
#include <sys/wait.h>
#include <filesystem>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

namespace apr_system {

// RAII guard for automatic directory restoration (single-thread assumption)
class DirectoryGuard {
public:
    explicit DirectoryGuard(const std::string& new_dir) : original_path_(getcwd(nullptr, 0)) {
        if (!new_dir.empty() && new_dir != "." && chdir(new_dir.c_str()) != 0) {
            throw std::runtime_error("Failed to change directory to: " + new_dir);
        }
    }
    ~DirectoryGuard() {
        if (original_path_) {
            (void)chdir(original_path_);
            free(original_path_);
        }
    }
    DirectoryGuard(const DirectoryGuard&) = delete;
    DirectoryGuard& operator=(const DirectoryGuard&) = delete;
    DirectoryGuard(DirectoryGuard&&) = delete;
    DirectoryGuard& operator=(DirectoryGuard&&) = delete;
private:
    char* original_path_;
};

std::vector<ValidationResult> Validator::validatePatches(
    const std::vector<PatchCandidate>& prioritized_patches,
    const RepositoryMetadata& repo_metadata,
    int top_k
) {
    const auto validation_start_time = std::chrono::high_resolution_clock::now();

    LOG_COMPONENT_INFO("validator", "starting validation: {} patches, top-{}, budget: {}min, early_exit: {}",
        prioritized_patches.size(), top_k, config_.time_budget_minutes, config_.enable_early_exit);

    const auto patches_to_validate = std::min({
        top_k,
        config_.max_patches_to_validate,
        static_cast<int>(prioritized_patches.size())
    });

    std::vector<ValidationResult> results;
    results.reserve(patches_to_validate);

    for (int i = 0; i < patches_to_validate; ++i) {
        const auto& patch = prioritized_patches[i];

        if (isTimeBudgetExceeded(validation_start_time)) {
            LOG_COMPONENT_WARN("validator", "time budget exceeded, stopping validation");
            break;
        }

        LOG_COMPONENT_INFO("validator", "[{}] validating patch {}/{}: {} ({}:{})",
            patch.patch_id, i + 1, patches_to_validate, patch.patch_id, patch.file_path, patch.start_line);

        auto result = validatePatchTwoPhase(patch, repo_metadata, validation_start_time);
        results.emplace_back(std::move(result));

        if (config_.enable_early_exit && results.back().tests_passed) {
            LOG_COMPONENT_INFO("validator", "[{}] early exit, found plausible patch [SUCCESS]", results.back().patch_id);
            break;
        }
    }

    recordTotalValidationTime(validation_start_time);
    LOG_COMPONENT_INFO("validator", "validation completed: {}ms, {} results",
        phase_timing_.total_time_ms, results.size());

    return results;
}

void Validator::recordTotalValidationTime(const std::chrono::high_resolution_clock::time_point& start_time) {
    const auto end_time = std::chrono::high_resolution_clock::now();
    phase_timing_.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}

ValidationResult Validator::validatePatchTwoPhase(const PatchCandidate& patch,
                                                  const RepositoryMetadata& repo_metadata,
                                                  const std::chrono::high_resolution_clock::time_point& validation_start_time) {
    LOG_COMPONENT_INFO("validator", "[{}] PHASE A: validating against originally failing tests", patch.patch_id);
    auto phase_a_result = timedValidation([&]() {
        return validateFailingTests(patch, repo_metadata, validation_start_time);
    }, phase_timing_.phase_a_time_ms);

    if (!phase_a_result.compilation_success || !phase_a_result.tests_passed) {
        LOG_COMPONENT_WARN("validator", "[{}] PHASE A failed, patch doesn't fix originally failing tests", patch.patch_id);
        return phase_a_result;
    }

    LOG_COMPONENT_INFO("validator", "[{}] PHASE A passed, running PHASE B", patch.patch_id);
    LOG_COMPONENT_INFO("validator", "[{}] PHASE B: running full regression test suite", patch.patch_id);

    auto phase_b_result = timedValidation([&]() {
        return validateRegressionTests(patch, repo_metadata, phase_a_result, validation_start_time);
    }, phase_timing_.phase_b_time_ms);

    if (phase_b_result.tests_passed) {
        LOG_COMPONENT_INFO("validator", "[{}] PHASE B passed, patch is plausible", patch.patch_id);
    } else {
        LOG_COMPONENT_WARN("validator", "[{}] PHASE B failed, patch introduces regressions", patch.patch_id);
    }

    return phase_b_result;
}

template<typename ValidationFunc>
ValidationResult Validator::timedValidation(ValidationFunc&& func, long long& timing_accumulator) {
    const auto start = std::chrono::high_resolution_clock::now();
    auto result = std::forward<ValidationFunc>(func)();
    const auto end = std::chrono::high_resolution_clock::now();
    timing_accumulator += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
}

ValidationResult Validator::validateFailingTests(const PatchCandidate& patch,
                                                 const RepositoryMetadata& repo_metadata,
                                                 const std::chrono::high_resolution_clock::time_point& validation_start_time) {
    ValidationResult result{
        .patch_id = patch.patch_id,
        .compilation_success = false,
        .tests_passed = false,
        .build_time_ms = 0,
        .test_time_ms = 0,
        .build_output = "",
        .test_output = "",
        .error_message = "",
        .tests_passed_count = 0,
        .tests_total_count = 0,
        .phase_a_artifact_path = "",
        .phase_b_artifact_path = ""
    };

    const std::string repo_root = resolveRepoPathForPatch(patch);

    try {
        LOG_COMPONENT_INFO("validator", "[{}] PHASE A step 1: applying patch", patch.patch_id);
        if (!applyPatch(patch, repo_root)) {
            result.error_message = "Failed to apply patch";
            return result;
        }

        if (isTimeBudgetExceeded(validation_start_time)) {
            result.error_message = "Time budget exceeded during build";
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        LOG_COMPONENT_INFO("validator", "[{}] PHASE A step 2: building project", patch.patch_id);
        auto build_start = std::chrono::high_resolution_clock::now();
        // choose build working directory: prefer ctest build dir if available, otherwise repo root
        auto select_ctest_dir = [&](const std::string& base) -> std::string {
            auto looks_like_ctest_dir = [](const std::filesystem::path& p) -> bool {
                std::error_code lec;
                if (!std::filesystem::exists(p, lec) || lec) return false;
                return std::filesystem::exists(p / "CTestTestfile.cmake", lec)
                    || std::filesystem::exists(p / "DartConfiguration.tcl", lec)
                    || std::filesystem::exists(p / "CTestConfig.cmake", lec)
                    || std::filesystem::exists(p / "Testing", lec);
            };
            std::vector<std::filesystem::path> candidates;
            candidates.emplace_back(base);
            candidates.emplace_back(std::filesystem::path(base) / "build");
            for (auto it = std::filesystem::directory_iterator(base); it != std::filesystem::directory_iterator(); ++it) {
                if (it->is_directory()) { candidates.emplace_back(it->path() / "build"); }
            }
            int max_depth = 3; std::error_code rec_ec;
            for (auto it = std::filesystem::recursive_directory_iterator(base, rec_ec); it != std::filesystem::recursive_directory_iterator(); ++it) {
                if (rec_ec) {
                    break;
                }
                if (it.depth() > max_depth) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (it->is_regular_file() && it->path().filename() == "CTestTestfile.cmake") {
                    candidates.emplace_back(it->path().parent_path());
                }
            }
            for (const auto& cand : candidates) { if (looks_like_ctest_dir(cand)) return cand.string(); }
            return base;
        };
        std::string build_workdir = (repo_metadata.test_script.find("ctest") != std::string::npos)
            ? select_ctest_dir(repo_root)
            : repo_root;
        auto build_res_pair = buildProject(build_workdir, repo_metadata.build_script, validation_start_time);
        auto build_end = std::chrono::high_resolution_clock::now();

        result.build_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(build_end - build_start).count();
        result.build_output = build_res_pair.second;
        result.compilation_success = build_res_pair.first;

        if (!build_res_pair.first) {
            result.error_message = "Compilation failed: " + build_res_pair.second;
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        if (isTimeBudgetExceeded(validation_start_time)) {
            result.error_message = "Time budget exceeded during tests";
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        LOG_COMPONENT_INFO("validator", "[{}] PHASE A step 3: running originally failing tests", patch.patch_id);
        auto test_start = std::chrono::high_resolution_clock::now();
        // run tests
        TestRunResult tr = runGTests(repo_root, repo_metadata.test_script, patch.affected_tests,
                                     validation_start_time, "phase-a", patch.patch_id);
        auto test_end = std::chrono::high_resolution_clock::now();

        result.test_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_end - test_start).count();
        result.test_output = tr.stdout_text;
        result.tests_passed = tr.success;
        result.phase_a_artifact_path = tr.artifact_path;

        // ensure artifact exists; otherwise treat as failure
        if (!std::filesystem::exists(result.phase_a_artifact_path)) {
            LOG_COMPONENT_WARN("validator", "test artifact not found: {}", result.phase_a_artifact_path);
            result.tests_passed = false;
        }

        auto test_counts = parseGTestResults(result.phase_a_artifact_path);
        result.tests_total_count = test_counts.first;
        result.tests_passed_count = tr.success ? test_counts.first : test_counts.second;

        if (!tr.success) {
            result.error_message = "Originally failing tests still fail";
        }

    } catch (const std::exception& e) {
        result.error_message = "Exception during validation: " + std::string(e.what());
    }

    bool restore_success = restoreOriginalCode(patch, repo_root);
    if (!restore_success) {
        LOG_COMPONENT_ERROR("validator", "failed to restore original code for patch");
        if (result.error_message.empty()) {
            result.error_message = "Failed to restore original code";
        }
    }

    return result;
}

ValidationResult Validator::validateRegressionTests(const PatchCandidate& patch,
                                                    const RepositoryMetadata& repo_metadata,
                                                    const ValidationResult& phase_a_result,
                                                    const std::chrono::high_resolution_clock::time_point& validation_start_time) {
    ValidationResult result = phase_a_result;
    const std::string repo_root = resolveRepoPathForPatch(patch);

    try {
        if (!applyPatch(patch, repo_root)) {
            result.error_message = "Failed to re-apply patch for PHASE B";
            result.tests_passed = false;
            return result;
        }

        if (isTimeBudgetExceeded(validation_start_time)) {
            result.error_message = "Time budget exceeded during Phase B build";
            result.tests_passed = false;
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        // choose build working directory for phase B as well
        std::string build_workdir_b = repo_root;
        if (repo_metadata.test_script.find("ctest") != std::string::npos) {
            auto looks_like_ctest_dir_b = [](const std::filesystem::path& p) -> bool {
                std::error_code lec;
                if (!std::filesystem::exists(p, lec) || lec) return false;
                return std::filesystem::exists(p / "CTestTestfile.cmake", lec)
                    || std::filesystem::exists(p / "DartConfiguration.tcl", lec)
                    || std::filesystem::exists(p / "CTestConfig.cmake", lec)
                    || std::filesystem::exists(p / "Testing", lec);
            };
            std::vector<std::filesystem::path> candidates_b;
            candidates_b.emplace_back(repo_root);
            candidates_b.emplace_back(std::filesystem::path(repo_root) / "build");
            for (auto it = std::filesystem::directory_iterator(repo_root); it != std::filesystem::directory_iterator(); ++it) {
                if (it->is_directory()) { candidates_b.emplace_back(it->path() / "build"); }
            }
            int max_depth_b = 3; std::error_code rec_ec_b;
            for (auto it = std::filesystem::recursive_directory_iterator(repo_root, rec_ec_b); it != std::filesystem::recursive_directory_iterator(); ++it) {
                if (rec_ec_b) { break; }
                if (it.depth() > max_depth_b) { it.disable_recursion_pending(); continue; }
                if (it->is_regular_file() && it->path().filename() == "CTestTestfile.cmake") {
                    candidates_b.emplace_back(it->path().parent_path());
                }
            }
            for (const auto& cand : candidates_b) { if (looks_like_ctest_dir_b(cand)) { build_workdir_b = cand.string(); break; } }
        }
        auto build_res_pair = buildProject(build_workdir_b, repo_metadata.build_script, validation_start_time);
        if (!build_res_pair.first) {
            result.error_message = "PHASE B compilation failed: " + build_res_pair.second;
            result.tests_passed = false;
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        if (isTimeBudgetExceeded(validation_start_time)) {
            result.error_message = "Time budget exceeded during regression tests";
            result.tests_passed = false;
            restoreOriginalCode(patch, repo_root);
            return result;
        }

        auto test_start = std::chrono::high_resolution_clock::now();
        TestRunResult tr = runGTests(repo_root, repo_metadata.test_script, std::vector<std::string>{},
                                     validation_start_time, "phase-b", patch.patch_id);
        auto test_end = std::chrono::high_resolution_clock::now();

        long long regression_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_end - test_start).count();

        result.test_time_ms += regression_time_ms;
        result.test_output += "\n--- PHASE B Output ---\n" + tr.stdout_text;
        result.phase_b_artifact_path = tr.artifact_path;

        if (!std::filesystem::exists(result.phase_b_artifact_path)) {
            LOG_COMPONENT_WARN("validator", "test artifact not found: {}", result.phase_b_artifact_path);
            tr.success = false;
        }
        auto test_counts = parseGTestResults(result.phase_b_artifact_path);
        result.tests_total_count += test_counts.first;
        result.tests_passed = tr.success;

        if (tr.success) {
            result.tests_passed_count = result.tests_total_count;
        } else {
            result.tests_passed_count += test_counts.second;
            result.error_message = "Regression tests failed";
        }

    } catch (const std::exception& e) {
        result.error_message = "Exception during PHASE B: " + std::string(e.what());
        result.tests_passed = false;
    }

    restoreOriginalCode(patch, repo_root);
    return result;
}

bool Validator::applyPatch(const PatchCandidate& patch, const std::string& repo_path) {
    try {
        const auto full_file_path = repo_path + "/" + patch.file_path;

        if (!fileExists(full_file_path)) {
            LOG_COMPONENT_ERROR("validator", "file does not exist: {}", full_file_path);
            return false;
        }

        auto lines = readFileLines(full_file_path);
        if (!lines) {
            return false;
        }

        if (!isValidLineRange(patch, lines->size())) {
            LOG_COMPONENT_ERROR("validator", "invalid line range: {}-{} for file with {} lines",
                patch.start_line, patch.end_line, lines->size());
            return false;
        }

        auto modified_lines = splitIntoLines(patch.modified_code);
        auto new_content = applyPatchToLines(*lines, modified_lines, patch);

        if (!writeFileLines(full_file_path, new_content)) {
            return false;
        }

        LOG_COMPONENT_INFO("validator", "patch applied successfully to {}", patch.file_path);
        return true;

    } catch (const std::exception& e) {
        LOG_COMPONENT_ERROR("validator", "exception applying patch: {}", e.what());
        return false;
    }
}

std::string Validator::resolveRepoPathForPatch(const PatchCandidate& patch) const {
    // try current and parent directories to find where patch.file_path exists
    // common layouts: running from repo root ("."), from build dir ("./build"), or deeper
    // maybe be not good, but works for mvp :)
    const std::vector<std::string> candidates = {
        ".",
        "..",
        "../..",
        "../../.."
    };

    for (const auto& base : candidates) {
        std::filesystem::path candidate = std::filesystem::path(base) / patch.file_path;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            // return normalized base path as string
            return std::filesystem::path(base).lexically_normal().string();
        }
    }

    // fallback to current directory if not found
    return ".";
}

bool Validator::restoreOriginalCode(const PatchCandidate& patch, const std::string& repo_path) {
    try {
        std::string full_file_path = repo_path + "/" + patch.file_path;

        if (fileExists(repo_path + "/.git")) {
            std::string git_cmd = "git restore --source=HEAD -- " + patch.file_path;
            ExecResult git_res = executeCommand(git_cmd, repo_path, -1);
            if (git_res.ok) {
                LOG_COMPONENT_INFO("validator", "original code restored via git for {}", patch.file_path);
                return true;
            } else {
                LOG_COMPONENT_WARN("validator", "git restoration failed, falling back to manual method: {}", git_res.output);
            }
        }

        struct stat buffer;
        if (stat(full_file_path.c_str(), &buffer) != 0) {
            LOG_COMPONENT_ERROR("validator", "file does not exist for restoration");
            return false;
        }

        std::ifstream file(full_file_path);
        if (!file.is_open()) {
            LOG_COMPONENT_ERROR("validator", "failed to open file for restoration");
            return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();

        std::vector<std::string> original_lines;
        std::istringstream original_stream(patch.original_code);
        std::string original_line;
        while (std::getline(original_stream, original_line)) {
            original_lines.push_back(original_line);
        }

        std::istringstream modified_stream(patch.modified_code);
        int modified_line_count = 0;
        std::string temp_line;
        while (std::getline(modified_stream, temp_line)) {
            modified_line_count++;
        }

        std::vector<std::string> restored_content;

        for (int i = 0; i < patch.start_line - 1; ++i) {
            restored_content.push_back(lines[i]);
        }

        for (size_t i = 0; i < original_lines.size(); ++i) {
            restored_content.push_back(original_lines[i]);
        }

        int skip_lines = patch.start_line - 1 + modified_line_count;
        for (int i = skip_lines; i < static_cast<int>(lines.size()); ++i) {
            restored_content.push_back(lines[i]);
        }

        std::ofstream out_file(full_file_path);
        if (!out_file.is_open()) {
            LOG_COMPONENT_ERROR("validator", "failed to open file for restoration writing");
            return false;
        }

        for (size_t i = 0; i < restored_content.size(); ++i) {
            out_file << restored_content[i];
            if (i < restored_content.size() - 1) {
                out_file << "\n";
            }
        }
        if (!restored_content.empty()) {
            out_file << "\n";
        }
        out_file.close();

        LOG_COMPONENT_INFO("validator", "original code restored successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_COMPONENT_ERROR("validator", "exception restoring original code");
        return false;
    }
}

std::pair<bool, std::string> Validator::buildProject(const std::string& repo_path, const std::string& build_script,
                                                     const std::chrono::high_resolution_clock::time_point& validation_start_time) {
    if (build_script.empty()) {
        LOG_COMPONENT_WARN("validator", "no build script provided, skipping build");
        return std::make_pair(true, "No build script provided");
    }

    LOG_COMPONENT_DEBUG("validator", "executing build script");

    long long remaining_ms = getRemainingTimeBudgetMs(validation_start_time);
    if (remaining_ms <= 0) {
        return std::make_pair(false, "Time budget exceeded before build");
    }

    ExecResult res = executeCommand(build_script, repo_path, remaining_ms);
    return {res.ok, res.output};
}

// NOTE: returns TestRunResult with artifact path ALWAYS set
TestRunResult Validator::runGTests(const std::string& repo_path,
                                   const std::string& test_binary,
                                   const std::vector<std::string>& test_filter,
                                   const std::chrono::high_resolution_clock::time_point& validation_start_time,
                                   const std::string& phase_name,
                                   const std::string& patch_id) {
    TestRunResult tr;

    if (test_binary.empty()) {
        LOG_COMPONENT_ERROR("validator", "no gtest binary provided");
        tr.success = false;
        tr.stdout_text = "No gtest binary provided";
        tr.exit_code = -1;
        tr.artifact_path.clear();
        return tr;
    }

    // artifact under repo_path/artifacts/gtest/<phase>-<patch>.xml (absolute path)
    std::filesystem::path artifact_dir = std::filesystem::absolute(std::filesystem::path(repo_path) / "artifacts" / "gtest");
    std::error_code ec;
    std::filesystem::create_directories(artifact_dir, ec);
    if (ec) {
        LOG_COMPONENT_WARN("validator", "failed to create artifact dir '{}': {}", artifact_dir.string(), ec.message());
    }
    std::filesystem::path artifact_path_fs = artifact_dir / (phase_name + "-" + patch_id + ".xml");
    tr.artifact_path = artifact_path_fs.string(); // absolute path, set UNCONDITIONALLY

    // build command; support both direct gtest binary and ctest
    std::string command = test_binary; // allow full command string
    const bool is_ctest = (test_binary.find("ctest") != std::string::npos);

    // if using ctest, try to locate a working directory that contains ctest metadata
    std::string test_working_dir = repo_path;
    if (is_ctest) {
        auto looks_like_ctest_dir = [](const std::filesystem::path& p) -> bool {
            std::error_code lec;
            if (!std::filesystem::exists(p, lec) || lec) return false;
            return std::filesystem::exists(p / "CTestTestfile.cmake", lec)
                || std::filesystem::exists(p / "DartConfiguration.tcl", lec)
                || std::filesystem::exists(p / "CTestConfig.cmake", lec)
                || std::filesystem::exists(p / "Testing", lec);
        };

        // candidates: repo_path, repo_path/build, any immediate subdir named build, and a shallow recursive scan
        std::vector<std::filesystem::path> candidates;
        candidates.emplace_back(repo_path);
        candidates.emplace_back(std::filesystem::path(repo_path) / "build");

        // add subdir/build for first level subdirs
        for (auto it = std::filesystem::directory_iterator(repo_path); it != std::filesystem::directory_iterator(); ++it) {
            if (it->is_directory()) {
                candidates.emplace_back(it->path() / "build");
            }
        }

        // shallow recursive scan for CTestTestfile.cmake up to depth 3
        int max_depth = 3;
        std::error_code rec_ec;
        for (auto it = std::filesystem::recursive_directory_iterator(repo_path, rec_ec); it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (rec_ec) break;
            if (it.depth() > max_depth) { it.disable_recursion_pending(); continue; }
            if (it->is_regular_file() && it->path().filename() == "CTestTestfile.cmake") {
                candidates.emplace_back(it->path().parent_path());
                // don't break; collect several options
            }
        }

        for (const auto& cand : candidates) {
            if (looks_like_ctest_dir(cand)) { test_working_dir = cand.string(); break; }
        }
        LOG_COMPONENT_DEBUG("validator", "ctest working directory: {}", test_working_dir);
    }

    if (is_ctest) {
        // use CTest with junit output to our artifact path
        // optional regex filter via -R (join test names with '|')
        if (!test_filter.empty()) {
            std::string regex;
            for (size_t i = 0; i < test_filter.size(); ++i) {
                if (i > 0) regex += "|";
                regex += test_filter[i];
            }
            command += " -R \"" + regex + "\"";
        } else {
            LOG_COMPONENT_DEBUG("validator", "running full test suite");
        }
        // ensure failures are visible and emit junit xml
        command += " --output-on-failure --output-junit \"" + tr.artifact_path + "\"";
    } else {
        // gtest binary: use --gtest_filter and --gtest_output
        if (!test_filter.empty()) {
            LOG_COMPONENT_DEBUG("validator", "running specific failing tests with --gtest_filter");
            command += " --gtest_filter=";
            for (size_t i = 0; i < test_filter.size(); ++i) {
                command += test_filter[i];
                if (i < test_filter.size() - 1) {
                    command += ":";
                }
            }
        } else {
            LOG_COMPONENT_DEBUG("validator", "running full test suite");
        }
        command += " --gtest_output=xml:\"" + tr.artifact_path + "\"";
    }

    long long remaining_ms = getRemainingTimeBudgetMs(validation_start_time);
    if (remaining_ms <= 0) {
        tr.success = false;
        tr.stdout_text = "Time budget exceeded before tests";
        tr.exit_code = -1;
        return tr;
    }

    ExecResult res = executeCommand(command, test_working_dir, remaining_ms);
    
    // if command succeeded but artifact wasn't created, mark failure explicitly
    if (res.ok) {
        std::error_code fec;
        if (!std::filesystem::exists(tr.artifact_path, fec)) {
            LOG_COMPONENT_WARN("validator", "expected test artifact not created: {}", tr.artifact_path);
            tr.success = false;
            tr.stdout_text = res.output + "\n[validator] expected test artifact not created: " + tr.artifact_path;
            tr.exit_code = (tr.exit_code == 0 ? 1 : tr.exit_code);
            return tr;
        }
    }
    tr.success = res.ok;
    tr.stdout_text = std::move(res.output);
    tr.exit_code = res.exit_code;
    return tr;
}

ExecResult Validator::executeCommand(const std::string& command,
                                     const std::string& working_dir,
                                     long long timeout_ms) {
    ExecResult result;
    const auto t0 = std::chrono::high_resolution_clock::now();

    // pipe for capturing stdout+stderr
    int pipefd[2];
#if defined(O_CLOEXEC)
    if (pipe2(pipefd, O_CLOEXEC) == -1) {
        result.ok = false; result.exit_code = -1;
        result.output = "pipe2 failed: " + std::string(strerror(errno));
        return result;
    }
#else
    if (pipe(pipefd) == -1) {
        result.ok = false; result.exit_code = -1;
        result.output = "pipe failed: " + std::string(strerror(errno));
        return result;
    }
    fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
#endif

    pid_t pid = fork();
    if (pid < 0) {
        // fork failed
        close(pipefd[0]); close(pipefd[1]);
        result.ok = false; result.exit_code = -1;
        result.output = "fork failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // ---- child ----
        // own process group so we can kill the whole tree
        setpgid(0, 0);

        // move to working_dir (best-effort)
        if (!working_dir.empty() && working_dir != "." && chdir(working_dir.c_str()) != 0) {
            _exit(127); // chdir failed
        }

        // redirect stdout & stderr -> pipe write end
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]); close(pipefd[1]);

        // exec via sh -lc so we can pass a shell command string
        execl("/bin/sh", "sh", "-lc", command.c_str(), (char*)nullptr);
        _exit(127); // exec failed
    }

    // ---- parent ----
    close(pipefd[1]);

    // nonblocking read
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    std::string buffer;
    buffer.reserve(16 * 1024);
    const int kill_grace_ms = 5000; // after TERM, wait this long then KILL
    bool timed_out = false;
    bool child_exited = false;
    int status = 0;

    auto time_left = [&](long long extra = 0) -> int {
        if (timeout_ms < 0) return -1; // infinite
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::high_resolution_clock::now() - t0)
                           .count();
        long long left = timeout_ms - elapsed - extra;
        return (left > 0) ? static_cast<int>(left) : 0;
    };

    while (true) {
        // check child status without blocking
        pid_t wp = waitpid(pid, &status, WNOHANG);
        if (wp == pid) {
            child_exited = true;
        }

        // drain pipe
        char chunk[4096];
        while (true) {
            ssize_t n = read(pipefd[0], chunk, sizeof(chunk));
            if (n > 0) buffer.append(chunk, static_cast<size_t>(n));
            else if (n == -1 && errno == EAGAIN) break; // nothing right now
            else break; // EOF or other error
        }

        if (child_exited) break;

        // handle timeout
        int wait_ms = time_left();
        if (timeout_ms >= 0 && wait_ms == 0) {
            timed_out = true;
            // send SIGTERM to the whole process group
            kill(-pid, SIGTERM);

            // poll pipe during grace period
            int remaining = kill_grace_ms;
            while (remaining > 0) {
                struct pollfd pfd{pipefd[0], POLLIN, 0};
                int step = std::min(remaining, 200);
                int pr = poll(&pfd, 1, step);
                if (pr > 0 && (pfd.revents & POLLIN)) {
                    ssize_t n = read(pipefd[0], chunk, sizeof(chunk));
                    if (n > 0) buffer.append(chunk, static_cast<size_t>(n));
                }
                // did child exit?
                wp = waitpid(pid, &status, WNOHANG);
                if (wp == pid) { child_exited = true; break; }
                remaining -= step;
            }

            if (!child_exited) {
                kill(-pid, SIGKILL);
                // ensure reaped
                waitpid(pid, &status, 0);
            }
            break;
        }

        // poll a bit to avoid busy loop (also covers infinite timeout)
        struct pollfd pfd{pipefd[0], POLLIN, 0};
        int pr = poll(&pfd, 1, (timeout_ms >= 0) ? std::min(wait_ms, 200) : 200);
        (void)pr; // nothing else to do; we loop and read above
    }

    // final drain
    while (true) {
        char chunk[4096];
        ssize_t n = read(pipefd[0], chunk, sizeof(chunk));
        if (n > 0) buffer.append(chunk, static_cast<size_t>(n));
        else break;
    }
    close(pipefd[0]);

    // fill result
    result.output = std::move(buffer);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = -1;
    }

    if (timed_out) {
        result.ok = false;
        result.output = "Command timed out and was terminated\n" + result.output;
    } else {
        result.ok = (result.exit_code == 0);
    }

    return result;
}

bool Validator::isTimeBudgetExceeded(const std::chrono::high_resolution_clock::time_point& start_time) const {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::minutes>(current_time - start_time);
    return elapsed_time.count() >= config_.time_budget_minutes;
}

long long Validator::getRemainingTimeBudgetMs(const std::chrono::high_resolution_clock::time_point& start_time) const {
    auto current_time = std::chrono::high_resolution_clock::now();
    long long elapsed_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
    long long budget_ms = static_cast<long long>(config_.time_budget_minutes) * 60 * 1000;
    long long remaining_ms = budget_ms - elapsed_time_ms;
    return std::max(0LL, remaining_ms);
}

bool Validator::fileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

std::optional<std::vector<std::string>> Validator::readFileLines(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_COMPONENT_ERROR("validator", "failed to open file: {}", path);
        return std::nullopt;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.emplace_back(std::move(line));
    }
    return lines;
}

bool Validator::writeFileLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_COMPONENT_ERROR("validator", "failed to open file for writing: {}", path);
        return false;
    }
    for (const auto& line : lines) {
        file << line << '\n';
    }
    return true;
}

std::vector<std::string> Validator::splitIntoLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.emplace_back(std::move(line));
    }
    return lines;
}

bool Validator::isValidLineRange(const PatchCandidate& patch, size_t file_size) {
    return patch.start_line >= 1 &&
           patch.end_line >= patch.start_line &&
           patch.end_line <= static_cast<int>(file_size);
}

std::vector<std::string> Validator::applyPatchToLines(
    const std::vector<std::string>& original_lines,
    const std::vector<std::string>& modified_lines,
    const PatchCandidate& patch) {

    // perform column-sensitive in-line replacement for single-line edits (mvp)
    if (patch.start_line == patch.end_line && !patch.original_code.empty() && !modified_lines.empty()) {
        const int line_idx = patch.start_line - 1;
        const bool valid_index = line_idx >= 0 && line_idx < static_cast<int>(original_lines.size());

        auto try_inline_replace = [&](int idx) -> std::optional<std::vector<std::string>> {
            const std::string &orig_line = original_lines[idx];
            if (const size_t pos = orig_line.find(patch.original_code); pos != std::string::npos) {
                std::vector<std::string> updated_lines = original_lines;
                std::string replaced_line = orig_line;
                replaced_line.replace(pos, patch.original_code.size(), modified_lines.front());
                updated_lines[idx] = std::move(replaced_line);
                LOG_COMPONENT_INFO(
                    "validator",
                    "[{}] in-line replacement at {}:{}: '{}' -> '{}'",
                    patch.patch_id,
                    patch.file_path,
                    patch.start_line,
                    patch.original_code,
                    modified_lines.front()
                );
                return updated_lines;
            }
            return std::nullopt;
        };

        if (valid_index) {
            if (auto replaced = try_inline_replace(line_idx)) {
                return std::move(*replaced);
            }
        }
    }
    // if inline replacement conditions aren't met, return original lines unchanged
    return original_lines;
}

// parse gtest xml output to extract (total, passed), minimal MVP version
std::pair<int, int> Validator::parseGTestResults(const std::string& xml_path) {
    int total = 0;
    int passed = 0;

    if (xml_path.empty()) {
        return {0, 0};
    }

    std::ifstream xml_file(xml_path);
    if (!xml_file.is_open()) {
        LOG_COMPONENT_WARN("validator", "failed to open gtest xml file: {}", xml_path);
        return {0, 0};
    }

    std::string line;
    while (std::getline(xml_file, line)) {
        if (line.find("<testsuites") != std::string::npos) {
            auto get_attr = [&](const std::string& key) -> std::optional<int> {
                auto pos = line.find(key + "=\"");
                if (pos == std::string::npos) return std::nullopt;
                size_t start = pos + key.size() + 2;
                size_t end = line.find("\"", start);
                if (end == std::string::npos) return std::nullopt;
                try { return std::stoi(line.substr(start, end - start)); }
                catch (...) { return std::nullopt; }
            };

            total = get_attr("tests").value_or(0);
            int failures = get_attr("failures").value_or(0);
            int errors = get_attr("errors").value_or(0);
            int disabled = get_attr("disabled").value_or(0);
            passed = std::max(0, total - failures - errors - disabled);
            break;
        }
    }
    xml_file.close();

    if (total == 0) { total = 1; passed = 1; } // conservative fallback
    return {total, passed};
}

} // namespace apr_system
