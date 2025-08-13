#pragma once

#include "types.h"
#include <memory>
#include <vector>

namespace apr_system {

/**
 * @brief interface for sbfl (spectrum-based fault localization) component
 */
class ISBFL {
public:
  virtual ~ISBFL() = default;

  /**
   * @brief run fault localization on test results and coverage data
   * @param sbfl_json path to json containing SBFL scores 
   * @return vector of suspicious locations ranked by suspiciousness score
   */
  virtual std::vector<SuspiciousLocation>
  localizeFaults(const std::string& sbfl_json) = 0;
    /**
   * @brief runs SBFL analysis
   * @param buggy_program_dir Path to buggy program
   * @param sbfl_json sbfl_json to be updated
   * 
   * @return void, Updates sbfl_json
   */
  virtual void 
  runSBFLAnalysis(const std::string& buggy_program_dir, std::string& sbfl_json) = 0;
};

/**
 * @brief interface for ast parser component
 */
class IParser {
public:
  virtual ~IParser() = default;

  /**
   * @brief parse source files and extract ast nodes for suspicious locations
   * @param suspicious_locations locations identified by sbfl
   * @param source_files list of source files to parse
   * @return vector of ast nodes
   */
  virtual std::vector<ASTNode>
  parseAST(const std::vector<SuspiciousLocation> &suspicious_locations,
           const std::vector<std::string> &source_files) = 0;
};

/**
 * @brief interface for mutation generator component
 */
class IMutator {
public:
  virtual ~IMutator() = default;

  /**
   * @brief generate patch candidates based on ast nodes
   * @param ast_nodes ast nodes to mutate
   * @param source_files source file contents
   * @return vector of patch candidates
   */
  virtual std::vector<PatchCandidate>
  generatePatches(const std::vector<ASTNode> &ast_nodes,
                  const std::vector<std::string> &source_files) = 0;
};

/**
 * @brief interface for patch prioritizer component
 */
class IPrioritizer {
public:
  virtual ~IPrioritizer() = default;

  /**
   * @brief prioritize patch candidates based on various heuristics
   * @param patch_candidates list of patch candidates
   * @param test_results test execution results for feature extraction
   * @return vector of prioritized patches sorted by priority score
   */
  virtual std::vector<PatchCandidate>
  prioritizePatches(const std::vector<PatchCandidate> &patch_candidates,
                    const std::string &mutation_freq_json) = 0;
};

/**
 * @brief interface for patch validator component
 */
class IValidator {
public:
  virtual ~IValidator() = default;

  /**
   * @brief validate patch candidates by applying them and running tests
   * @param prioritized_patches list of prioritized patches to validate
   * @param repo_metadata repository metadata for build/test configuration
   * @param top_k number of top patches to validate
   * @return vector of validation results
   */
  virtual std::vector<ValidationResult>
  validatePatches(const std::vector<PatchCandidate> &prioritized_patches,
                  const RepositoryMetadata &repo_metadata, int top_k = 10) = 0;
};

/**
 * @brief interface for the main orchestrator component
 */
class IOrchestrator {
public:
  virtual ~IOrchestrator() = default;

  /**
   * @brief run the complete apr project pipeline
   * @param repo_metadata repository metadata
   * @param sbfl_json path to sbfl results
   * @return complete system state after pipeline execution
   */
  virtual SystemState runPipeline(const RepositoryMetadata &repo_metadata,
                                  const std::string &sbfl_json,
                                  const std::string &mutation_freq_json) = 0;

  /**
   * @brief set component dependencies
   */
  virtual void setComponents(std::unique_ptr<ISBFL> sbfl,
                             std::unique_ptr<IParser> parser,
                             std::unique_ptr<IMutator> mutator,
                             std::unique_ptr<IPrioritizer> prioritizer,
                             std::unique_ptr<IValidator> validator) = 0;
};

} // namespace apr_system
