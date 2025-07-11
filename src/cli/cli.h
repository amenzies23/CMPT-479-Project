#pragma once

#include "../core/types.h"
#include <string>
#include <vector>

namespace apr_system {

/**
 * @brief command-line arguments structure
 */
struct CLIArgs {
  std::string repo_url;
  std::string branch;
  std::string commit_hash;
  std::string sbfl_json;
  std::string output_dir;
  std::string config_file;
  int max_patches;
  double confidence_threshold;
  bool help;
  bool verbose;
};

/**
 * @brief cli argument parser
 */
class CLIParser {
public:
  CLIParser() = default;
  ~CLIParser() = default;

  /**
   * @brief parse command-line arguments
   * @param argc argument count
   * @param argv argument vector
   * @return parsed arguments
   */
  static CLIArgs parseArgs(int argc, char *argv[]);

  /**
   * @brief print help information
   */
  static void printHelp();

  /**
   * @brief validate arguments
   * @param args arguments to validate
   * @return true if valid, false otherwise
   */
  static bool validateArgs(const CLIArgs &args);

  /**
   * @brief create repository metadata from cli arguments
   * @param args cli arguments
   * @return repository metadata
   */
  static RepositoryMetadata createRepositoryMetadata(const CLIArgs &args);

  /**
   * @brief load test results from file
   * @param file_path path to test results file
   * @return vector of test results
   */
  static std::vector<TestResult> loadTestResults(const std::string &file_path);

  /**
   * @brief load coverage data from file
   * @param file_path path to coverage file
   * @return coverage data
   */
  static CoverageData loadCoverageData(const std::string &file_path);

private:
  /**
   * @brief find source files in common directories
   * @return vector of source file paths
   */
  static std::vector<std::string> findSourceFiles();

  /**
   * @brief create mock coverage data for demo purposes
   * @return mock coverage data
   */
  static CoverageData createMockCoverageData();
};

} // namespace apr_system
