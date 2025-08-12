#include <iostream>
#include <memory>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>

#include "core/logger.h"
#include "orchestrator/orchestrator.h"
#include "sbfl/sbfl.h"
#include "parser/parser.h"
#include "mutator/mutator.h"
#include "prioritizer/prioritizer.h"
#include "validator/validator.h"
#include "cli/cli.h"

using namespace apr_system;

// fallback mock data for when files don't exist (just for testing)
std::vector<TestResult> createMockTestResults() {
    return {
        {
            .test_name = "test_hello_world",
            .passed = false,
            .execution_time_ms = 150,
            .output = "Expected: 'Hello, World!'\nGot:      'Hello World!'\nTest FAILED",
            .error_message = "Output mismatch: expected 'Hello, World!' but got 'Hello World!'"
        },
        {
            .test_name = "test_calculator_addition",
            .passed = false,
            .execution_time_ms = 89,
            .output = "Test: add(8, 2)\nExpected: 10\nActual: 8\nFAILED",
            .error_message = "Calculator addition function returns incorrect result"
        }
    };
}

CoverageData createMockCoverageData() {
    return {
        .line_coverage = {
            {
                .file_path = "src/hello_world.cpp",
                .line_number = 4,
                .hit_count = 1,
                .covered = true
            },
            {
                .file_path = "src/calculator.cpp",
                .line_number = 5,
                .hit_count = 3,
                .covered = true
            }
        },
        .covered_files = {"src/hello_world.cpp", "src/calculator.cpp"},
        .total_coverage_percentage = 78.5
    };
}

// function to save system state as JSON
void saveSystemStateToJSON(const SystemState& state, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open output file: " + filepath);
    }

    file << "{\n";
    file << "  \"pipeline_summary\": {\n";
    file << "    \"suspicious_locations_count\": " << state.suspicious_locations.size() << ",\n";
    file << "    \"ast_nodes_count\": " << state.ast_nodes.size() << ",\n";
    file << "    \"patch_candidates_count\": " << state.patch_candidates.size() << ",\n";
    file << "    \"prioritized_patches_count\": " << state.prioritized_patches.size() << ",\n";
    file << "    \"validation_results_count\": " << state.validation_results.size() << "\n";
    file << "  },\n";

    // repository metadata
    file << "  \"repository_metadata\": {\n";
    file << "    \"repository_url\": \"" << state.repo_metadata.repository_url << "\",\n";
    file << "    \"branch\": \"" << state.repo_metadata.branch << "\",\n";
    file << "    \"commit_hash\": \"" << state.repo_metadata.commit_hash << "\",\n";
    file << "    \"source_files_count\": " << state.repo_metadata.source_files.size() << "\n";
    file << "  },\n";

    // suspicious locations
    file << "  \"suspicious_locations\": [\n";
    for (size_t i = 0; i < state.suspicious_locations.size(); ++i) {
        const auto& loc = state.suspicious_locations[i];
        file << "    {\n";
        file << "      \"file_path\": \"" << loc.file_path << "\",\n";
        file << "      \"line_number\": " << loc.line_number << ",\n";
        file << "      \"suspiciousness_score\": " << loc.suspiciousness_score << ",\n";
        file << "      \"function\": \"" << loc.function << "\"\n";
        file << "    }";
        if (i < state.suspicious_locations.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    // validation results
    file << "  \"validation_results\": [\n";
    for (size_t i = 0; i < state.validation_results.size(); ++i) {
        const auto& result = state.validation_results[i];
        file << "    {\n";
        file << "      \"patch_id\": \"" << result.patch_id << "\",\n";
        file << "      \"compilation_success\": " << (result.compilation_success ? "true" : "false") << ",\n";
        file << "      \"tests_passed\": " << (result.tests_passed ? "true" : "false") << ",\n";
        file << "      \"tests_passed_count\": " << result.tests_passed_count << ",\n";
        file << "      \"tests_total_count\": " << result.tests_total_count << ",\n";
        file << "      \"build_time_ms\": " << result.build_time_ms << ",\n";
        file << "      \"test_time_ms\": " << result.test_time_ms << "\n";
        file << "    }";
        if (i < state.validation_results.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]";

    file << "\n";

    file << "}\n";
    file.close();
}

int main(int argc, char* argv[]) {
    try {
        // initialize logging system first
            apr_system::Logger::initialize(
        apr_system::Logger::Level::INFO, // default log level
        true, // enable file logging
        "logs/apr_system.log"
        );

        LOG_INFO("starting APR project system pipeline...");

        // parse command-line arguments
        auto args = CLIParser::parseArgs(argc, argv);

        if (args.help) {
            CLIParser::printHelp();
            return 0;
        }

        if (!CLIParser::validateArgs(args)) {
            LOG_ERROR("invalid arguments. use --help for usage information.");
            return 1;
        }

        if (args.verbose) {
            // enable debug logging in verbose mode
            apr_system::Logger::setLevel(apr_system::Logger::Level::DEBUG);
            LOG_INFO("verbose mode enabled");
            LOG_INFO("repository URL: {}", args.repo_url);
            LOG_INFO("branch: {}", args.branch);
            LOG_INFO("sbfl json: {}", args.sbfl_json);
            LOG_INFO("mutation frequency json: {}", args.mutation_freq_json);
        }

        // create component instances
        auto sbfl = std::make_unique<SBFL>();
        auto parser = std::make_unique<Parser>();
        auto mutator = std::make_unique<Mutator>();
        auto prioritizer = std::make_unique<Prioritizer>();
        auto validator = std::make_unique<Validator>();

        // create orchestrator and set components
        auto orchestrator = std::make_unique<Orchestrator>();
        orchestrator->setComponents(
            std::move(sbfl),
            std::move(parser),
            std::move(mutator),
            std::move(prioritizer),
            std::move(validator)
        );

        std::vector<TestResult> test_results;
        CoverageData coverage_data;

        try {
            if (std::filesystem::exists(args.sbfl_json)) {
                LOG_INFO("loading sbfl results from: {}", args.sbfl_json);
            } else {
                LOG_WARN("sbfl results file not found, using mock data");
                test_results = createMockTestResults();
            }

            if (std::filesystem::exists(args.mutation_freq_json)) {
                LOG_INFO("loading mutation frequencies from: {}", args.mutation_freq_json);
            }

        //     if (std::filesystem::exists(args.coverage_file)) {
        //         LOG_INFO("loading coverage data from: {}", args.coverage_file);
        //         coverage_data = CLIParser::loadCoverageData(args.coverage_file);
        //     } else {
        //         LOG_WARN("coverage file not found, using mock data");
        //         coverage_data = createMockCoverageData();
        //     }
        } catch (const std::exception& e) {
            LOG_WARN("error loading files, falling back to mock data: {}", e.what());
            test_results = createMockTestResults();
            coverage_data = createMockCoverageData();
        }

        // create repository metadata
        auto repo_metadata = CLIParser::createRepositoryMetadata(args);

        // if (args.verbose) {
        //     LOG_INFO("loaded {} test results", test_results.size());
        //     LOG_INFO("coverage: {:.1f}%", coverage_data.total_coverage_percentage);
        //     LOG_INFO("found {} source files", repo_metadata.source_files.size());
        // }

        // run the pipeline
        LOG_INFO("running APR project pipeline...");
        auto system_state = orchestrator->runPipeline(repo_metadata, args.sbfl_json, args.mutation_freq_json);

        // create output directory
        std::filesystem::create_directories(args.output_dir);

        // save results to output directory
        std::string output_file = args.output_dir + "/pipeline_results.json";
        LOG_INFO("saving results to: {}", output_file);

        try {
            saveSystemStateToJSON(system_state, output_file);
            LOG_INFO("results saved successfully!");
        } catch (const std::exception& e) {
            LOG_ERROR("failed to save results: {}", e.what());
        }

        // output summary
        LOG_INFO("pipeline completed successfully!");
        LOG_INFO("found {} suspicious locations", system_state.suspicious_locations.size());
        LOG_INFO("generated {} patch candidates", system_state.patch_candidates.size());
        LOG_INFO("validated {} patches", system_state.validation_results.size());

        // pr creation handled by github app layer

        // set exit code based on whether patches were found
        if (system_state.validation_results.empty()) {
            LOG_WARN("no valid patches generated");
            apr_system::Logger::shutdown();
            return 2; // no patches
        }

        apr_system::Logger::shutdown();
        return 0;

    } catch (const std::exception& e) {
        LOG_CRITICAL("fatal error: {}", e.what());
        apr_system::Logger::shutdown();
        return 1;
    }
}
