#pragma once

#include "../core/contracts.h"
#include <memory>

namespace apr_system {

/**
 * @brief implementation of SBFL (spectrum-based fault localization)
 *
 * Generates suspicious location scores
 */
class SBFL : public ISBFL {
public:
  SBFL() = default;
  ~SBFL() = default;

  /**
   * @brief generate suspicious location scores
   * 
   * Parse the SBFL json file to return a vector of suspicious locations
   * 
   * @param sbfl_json Path to json file containing SBFL scores
   * 
   * @return vector of suspicious locations ranked by suspiciousness score
   * (0.0-1.0)
   */
  std::vector<SuspiciousLocation> localizeFaults(const std::string& sbfl_json) override;


  /**
   * @brief runs SBFL analysis
   * 
   * Given a local buggy program directory run the SBFL analysis python script to generate
   * a json
   * 
   * @param buggy_program_dir Path to buggy program
   * @param sbfl_json sbfl_json to be updated
   * 
   * @return void, Updates sbfl_json
   */
  void runSBFLAnalysis(const std::string& buggy_program_dir, std::string& sbfl_json);
};

} // namespace apr_system
