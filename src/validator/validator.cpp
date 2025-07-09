#include "validator.h"
#include "../core/logger.h"

namespace apr_system {

std::vector<ValidationResult> Validator::validatePatches(
    const std::vector<PrioritizedPatch>& prioritized_patches,
    const RepositoryMetadata& repo_metadata,
    int top_k
) {
    LOG_COMPONENT_INFO("validator", "starting patch validation with {} prioritized patches, validating top {}",
                       prioritized_patches.size(), top_k);

    /*
     * TODO: implement patch validation here
     *
     * should:
     * 1. apply each patch to the source code
     * 2. build the modified code using repo_metadata.build_script
     * 3. run tests using repo_metadata.test_script
     * 4. measure execution times and collect results
     *
     * input contracts:
     * - prioritized_patches: patches sorted by priority from Prioritizer
     * - repo_metadata: repository information including build/test commands
     * - top_k: maximum number of patches to validate
     *
     * output contract:
     * - return ValidationResult objects with compilation and test results
     * - include timing information and detailed output/error messages
     */

    // mock data for testing data flow -> remove it when implementing
    std::vector<ValidationResult> mock_results;

    // validate top-k patches
    int patches_to_validate = std::min(top_k, static_cast<int>(prioritized_patches.size()));

    for (int i = 0; i < patches_to_validate; ++i) {
        const auto& patch = prioritized_patches[i];

        LOG_COMPONENT_DEBUG("validator", "validating patch {}/{}: {}", i + 1, patches_to_validate, patch.patch_id_ref);

        ValidationResult result{
            .patch_id = patch.patch_id_ref,
            .compilation_success = i < 2, // first 2 patches compile successfully
            .tests_passed = i == 0,       // only first patch passes tests
            .build_time_ms = 1000 + (i * 200),  // mock build times
            .test_time_ms = 500 + (i * 100),    // mock test times
            .build_output = "[STUB] mock build output for patch " + patch.patch_id_ref,
            .test_output = "[STUB] mock test output for patch " + patch.patch_id_ref,
            .error_message = i >= 2 ? "mock compilation error" : "",
            .tests_passed_count = i == 0 ? 5 : (i == 1 ? 3 : 0),
            .tests_total_count = 5
        };

        // log validation result for each patch using component-specific logging
        if (result.compilation_success) {
            if (result.tests_passed) {
                LOG_COMPONENT_INFO("validator", "patch {} - compilation successful, tests passed ({}/{})",
                                  result.patch_id, result.tests_passed_count, result.tests_total_count);
            } else {
                LOG_COMPONENT_WARN("validator", "patch {} - compilation successful, tests failed ({}/{})",
                                  result.patch_id, result.tests_passed_count, result.tests_total_count);
            }
        } else {
            LOG_COMPONENT_ERROR("validator", "patch {} - compilation failed: {}", result.patch_id, result.error_message);
        }

        mock_results.push_back(result);
    }

    LOG_COMPONENT_INFO("validator", "validation completed - returning {} results", mock_results.size());
    return mock_results;
}

} // namespace apr_system
