#include "sbfl.h"
#include "../core/logger.h"

namespace apr_system {

std::vector<SuspiciousLocation> SBFL::localizeFaults(
    const std::vector<TestResult>& test_results,
    const CoverageData& coverage_data
) {
    LOG_COMPONENT_INFO("sbfl", "this is a stub implementation");
    LOG_COMPONENT_INFO("sbfl", "input: {} test results, {} coverage lines",
                       test_results.size(), coverage_data.line_coverage.size());

    /*
     * TODO: implement fault localization algorithm here
     *
     * input contracts:
     * - test_results: array of test execution results with pass/fail status
     * - coverage_data: line-by-line coverage information
     *
     * output contract:
     * - return SuspiciousLocation objects sorted by suspiciousness score (descending)
     * - each location must have valid file_path, line_number, and score 0.0-1.0
     */

    // mock data for testing data flow -> remove it when implementing
    std::vector<SuspiciousLocation> mock_locations;

    // create mock suspicious locations based on input data
    for (size_t i = 0; i < std::min(size_t(3), coverage_data.line_coverage.size()); ++i) {
        const auto& line_cov = coverage_data.line_coverage[i];

        SuspiciousLocation location{
            .file_path = line_cov.file_path,
            .line_number = line_cov.line_number,
            .suspiciousness_score = 0.8 - (i * 0.2),
            .reason = "[STUB] mock suspicious location for testing data flow"
        };

        mock_locations.push_back(location);
    }

    LOG_COMPONENT_INFO("sbfl", "stub returning {} mock suspicious locations", mock_locations.size());
    return mock_locations;
}

} // namespace apr_system
