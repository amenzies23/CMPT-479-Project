#include "cli.h"

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../core/logger.h"

namespace apr_system {

CLIArgs CLIParser::parseArgs(int argc, char* argv[]) {
    CLIArgs args;

    // set some defaults
    args.repo_url = "";
    args.branch = "main";
    args.commit_hash = "";
    args.sbfl_json = "";
    // args.sbfl_json = std::string(PROJECT_SOURCE_DIR) + "/src/testing_mock/data.json";
    args.mutation_freq_json = std::string(PROJECT_SOURCE_DIR) + "/test-data/freq.json";
    args.output_dir = "apr-project-results";
    args.buggy_program_dir = "";
    args.max_patches = 5;
    args.confidence_threshold = 0.7;
    args.config_file = "";
    args.build_script = "";
    args.test_script = "";
    args.help = false;
    args.verbose = false;
    args.use_testing_mock = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--repo-url" && i + 1 < argc) {
            args.repo_url = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            args.output_dir = argv[++i];
        } else if (arg == "--buggy-program" && i + 1 < argc) {
            args.buggy_program_dir = argv[++i];
        } else if (arg == "--sbfl-json" && i + 1 < argc) {
            args.sbfl_json = argv[++i];
        } else if (arg == "--freq-json" && i + 1 < argc) {
            args.mutation_freq_json = argv[++i];
        } else if (arg == "--build" && i + 1 < argc) {
            args.build_script = argv[++i];
        } else if (arg == "--test" && i + 1 < argc) {
            args.test_script = argv[++i];
        } else if (arg == "--use-testing-mock") {
            args.use_testing_mock = true;
        }
    }

    if (args.use_testing_mock) {
        if (args.sbfl_json.empty()) {
            args.sbfl_json = std::string(PROJECT_SOURCE_DIR) + "/src/testing_mock/data.json";
        }

        // use testing_mock sources
        // build in a dedicated dir under repo root; validator will find ctest dir
        args.build_script = std::string("cmake -S ") + PROJECT_SOURCE_DIR + "/src/testing_mock -B build/testing_mock && cmake --build build/testing_mock -j";
        // run ctest from that build, validator will detect ctest dir even if working in repo root
        args.test_script = "ctest";
    }

    return args;
}

void CLIParser::printHelp() {
    std::cout << "APR system - automated program repair\n\n";
    std::cout << "usage: apr_system [OPTIONS]\n\n";
    std::cout << "options:\n";
    std::cout << "  --repo-url URL       repository URL to analyze\n";
    std::cout << "  --output-dir DIR     directory to store results (default: apr-project-results)\n";
    std::cout << " --buggy-program DIR   directory to the buggy program\n";
    std::cout << "  --sbfl-json PATH     path to SBFL results json\n";
    std::cout << "  --freq-json PATH     path to historical frequency json\n";
    std::cout << "  --build CMD          build command to compile project under test\n";
    std::cout << "  --test CMD           test command (ctest or gtest binary)\n";
    std::cout << "  --use-testing-mock   convenience flag to target src/testing_mock\n";
    std::cout << "  --verbose, -v        enable verbose output\n";
    std::cout << "  --help, -h           show this help message\n\n";
    std::cout << "example:\n";
    std::cout << "  apr_system --use-testing-mock --sbfl-json ../src/testing_mock/data.json --verbose\n";
}

bool CLIParser::validateArgs(const CLIArgs& args) {
    // simplified validation
    return true;
}

RepositoryMetadata CLIParser::createRepositoryMetadata(const CLIArgs& args) {
    RepositoryMetadata metadata;
    metadata.repository_url = args.repo_url.empty() ? "https://github.com/example/repo" : args.repo_url;
    metadata.branch = args.branch;
    metadata.commit_hash = args.commit_hash.empty() ? "abc123" : args.commit_hash;
    metadata.build_script = args.build_script.empty() ? "cmake .. && make" : args.build_script;
    metadata.test_script = args.test_script.empty() ? "ctest" : args.test_script;
    metadata.source_files = findSourceFiles(args.buggy_program_dir);

    // metadata.source_files.push_back("src/main.cpp");
    // metadata.source_files.push_back("src/hello_world.cpp");
//     if (args.use_testing_mock) {
//         metadata.source_files.push_back("src/testing_mock/src/calculator.cpp");
//     }

    return metadata;
}

std::vector<TestResult> CLIParser::loadTestResults(const std::string& file_path) {
    // simplified -> return mock data
    LOG_COMPONENT_INFO("cli", "loading test results (using mock data)");

    std::vector<TestResult> results;

    TestResult test1;
    test1.test_name = "test_calculator_add";
    test1.passed = false;
    test1.execution_time_ms = 120;
    test1.output = "8";
    test1.error_message = "Expected 10, got 8";
    results.push_back(test1);

    TestResult test2;
    test2.test_name = "test_hello_world_output";
    test2.passed = false;
    test2.execution_time_ms = 40;
    test2.output = "Hello world!";
    test2.error_message = "Expected Hello, world!";
    results.push_back(test2);

    return results;
}

CoverageData CLIParser::loadCoverageData(const std::string& file_path) {
    // simplified -> return mock data
    LOG_COMPONENT_INFO("cli", "loading coverage data (using mock data)");

    CoverageData coverage;

    LineCoverage line1;
    line1.file_path = "src/main.cpp";
    line1.line_number = 10;
    line1.hit_count = 8;
    line1.covered = true;
    coverage.line_coverage.push_back(line1);

    LineCoverage line2;
    line2.file_path = "src/hello_world.cpp";
    line2.line_number = 4;
    line2.hit_count = 1;
    line2.covered = true;
    coverage.line_coverage.push_back(line2);

    LineCoverage line3;
    line3.file_path = "src/calculator.cpp";
    line3.line_number = 5;
    line3.hit_count = 1;
    line3.covered = true;
    coverage.line_coverage.push_back(line3);

    coverage.covered_files.push_back("src/main.cpp");
    coverage.covered_files.push_back("src/hello_world.cpp");
    coverage.covered_files.push_back("src/calculator.cpp");
    coverage.total_coverage_percentage = 85.0;

    return coverage;
}

std::vector<std::string> CLIParser::findSourceFiles(const std::string& buggy_program_dir) {
    // simplified -> return static list
    const std::string buggy_program_src = buggy_program_dir + "/src";
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(buggy_program_src)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            files.push_back(entry.path().string());
        }
    }

    return files;
}

CoverageData CLIParser::createMockCoverageData() {
    // simplified -> return mock data
    CoverageData coverage;

    LineCoverage line1;
    line1.file_path = "src/main.cpp";
    line1.line_number = 10;
    line1.hit_count = 5;
    line1.covered = true;
    coverage.line_coverage.push_back(line1);

    LineCoverage line2;
    line2.file_path = "src/hello_world.cpp";
    line2.line_number = 4;
    line2.hit_count = 2;
    line2.covered = true;
    coverage.line_coverage.push_back(line2);

    LineCoverage line3;
    line3.file_path = "src/calculator.cpp";
    line3.line_number = 5;
    line3.hit_count = 1;
    line3.covered = true;
    coverage.line_coverage.push_back(line3);

    coverage.covered_files.push_back("src/main.cpp");
    coverage.covered_files.push_back("src/hello_world.cpp");
    coverage.covered_files.push_back("src/calculator.cpp");
    coverage.total_coverage_percentage = 80.0;

    return coverage;
}

}  // namespace apr_system
