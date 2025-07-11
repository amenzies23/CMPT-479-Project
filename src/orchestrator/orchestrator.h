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
   * @param test_results test execution results
   * @param coverage_data code coverage information
   * @return complete system state after pipeline execution
   */
  SystemState runPipeline(const RepositoryMetadata &repo_metadata,
                          const std::vector<TestResult> &test_results,
                          const CoverageData &coverage_data) override;

  /**
   * @brief set component dependencies
   */
  void setComponents(std::unique_ptr<ISBFL> sbfl,
                     std::unique_ptr<IParser> parser,
                     std::unique_ptr<IMutator> mutator,
                     std::unique_ptr<IPrioritizer> prioritizer,
                     std::unique_ptr<IValidator> validator,
                     std::unique_ptr<IPRBot> prbot) override;

private:
  std::unique_ptr<ISBFL> sbfl_;
  std::unique_ptr<IParser> parser_;
  std::unique_ptr<IMutator> mutator_;
  std::unique_ptr<IPrioritizer> prioritizer_;
  std::unique_ptr<IValidator> validator_;
  std::unique_ptr<IPRBot> prbot_;
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
