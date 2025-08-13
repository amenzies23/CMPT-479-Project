#pragma once

#include <filesystem>
#include <memory>

#include "../core/contracts.h"
// #include "../validator/json_schema_validator.h"  // TEMPORARILY DISABLED

namespace apr_system {

/**
 * @brief concrete implementation of the main orchestrator component
 *
 * coordinates the entire apr project pipeline by orchestrating all components.
 */
class Orchestrator : public IOrchestrator {
public:
  Orchestrator();
  ~Orchestrator() override = default;

  /**
   * @brief run the complete apr project pipeline
   * @param repo_metadata repository metadata
   * @param failed_tests_log path to CTest failed tests log file
   * @param coverage_base_dir path to directory containing .gcov files organized by test
   * @return complete system state after pipeline execution
   */
  SystemState runPipeline(const RepositoryMetadata &repo_metadata,
                          const std::string &sbfl_json,
                          const std::string &mutation_freq_json
                        ) override;

  /**
   * @brief set component dependencies
   */
  void setComponents(std::unique_ptr<ISBFL> sbfl,
                     std::unique_ptr<IParser> parser,
                     std::unique_ptr<IMutator> mutator,
                     std::unique_ptr<IPrioritizer> prioritizer,
                     std::unique_ptr<IValidator> validator) override;

private:
  std::unique_ptr<ISBFL> sbfl_;
  std::unique_ptr<IParser> parser_;
  std::unique_ptr<IMutator> mutator_;
  std::unique_ptr<IPrioritizer> prioritizer_;
  std::unique_ptr<IValidator> validator_;
  // std::unique_ptr<JSONSchemaValidator> schema_validator_;  // TEMPORARILY DISABLED

  /**
   * @brief validate that all components are set
   * 
   */
  void validateComponents() const;

  /**
   * @brief log pipeline step information
   * @param step_name name of the current step
   * @param details additional details about the step
   */
  void logPipelineStep(const std::string &step_name,
                       const std::string &details = "") const;
};

} // namespace apr_system
