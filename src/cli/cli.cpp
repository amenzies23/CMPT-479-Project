#include "cli.h"
#include "../core/logger.h"
#include <iostream>
#include <nlohmann/json.hpp>

namespace apr_system {

CLIArgs CLIParser::parseArgs(int argc, char* argv[]) {
    CLIArgs args;

    // set some defaults
    args.repo_url = "";
    args.branch = "main";
    args.commit_hash = "";
    args.test_results_file = "test-data/test_results.json";
    args.coverage_file = "test-data/coverage.xml";
    args.output_dir = "apr-project-results";
    args.max_patches = 5;
    args.confidence_threshold = 0.7;
    args.config_file = "";
    args.help = false;
    args.verbose = false;

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
        }
    }

    return args;
}

void CLIParser::printHelp() {
    std::cout << "APR system - automated program repair\n\n";
    std::cout << "usage: apr_system [OPTIONS]\n\n";
    std::cout << "options:\n";
    std::cout << "  --repo-url URL       repository URL to analyze\n";
    std::cout << "  --output-dir DIR     directory to store results (default: apr-project-results)\n";
    std::cout << "  --verbose, -v        enable verbose output\n";
    std::cout << "  --help, -h           show this help message\n\n";
    std::cout << "example:\n";
    std::cout << "  apr_system --repo-url https://github.com/user/repo --verbose\n";
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
    metadata.build_script = "cmake .. && make";
    metadata.test_script = "ctest";
    metadata.source_files.push_back("src/main.cpp");
    metadata.source_files.push_back("src/example.cpp");
    return metadata;
}

std::vector<TestResult> CLIParser::loadTestResults(const std::string& file_path) {
    // simplified -> return mock data instead of complex file parsing
    LOG_COMPONENT_INFO("cli", "loading test results (using mock data)");
    
    std::vector<TestResult> results;
    
    TestResult test1;
    test1.test_name = "test_example_function";
    test1.passed = false;
    test1.execution_time_ms = 150;
    test1.output = "";
    test1.error_message = "Expected 42, got 40";
    results.push_back(test1);
    
    TestResult test2;
    test2.test_name = "test_basic_functionality";
    test2.passed = true;
    test2.execution_time_ms = 50;
    test2.output = "Test passed successfully";
    test2.error_message = "";
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
    line2.file_path = "src/example.cpp";
    line2.line_number = 15;
    line2.hit_count = 2;
    line2.covered = true;
    coverage.line_coverage.push_back(line2);
    
    LineCoverage line3;
    line3.file_path = "src/example.cpp";
    line3.line_number = 20;
    line3.hit_count = 0;
    line3.covered = false;
    coverage.line_coverage.push_back(line3);
    
    coverage.covered_files.push_back("src/main.cpp");
    coverage.covered_files.push_back("src/example.cpp");
    coverage.total_coverage_percentage = 75.0;
    
    return coverage;
}

std::vector<std::string> CLIParser::findSourceFiles() {
    // simplified -> return static list
    std::vector<std::string> files;
    files.push_back("src/main.cpp");
    files.push_back("src/example.cpp");
    files.push_back("src/utils.cpp");
    return files;
}

CoverageData CLIParser::createMockCoverageData() {
    // simplified mock data
    CoverageData coverage;
    
    LineCoverage line1;
    line1.file_path = "src/main.cpp";
    line1.line_number = 10;
    line1.hit_count = 5;
    line1.covered = true;
    coverage.line_coverage.push_back(line1);
    
    LineCoverage line2;
    line2.file_path = "src/example.cpp";
    line2.line_number = 15;
    line2.hit_count = 2;
    line2.covered = true;
    coverage.line_coverage.push_back(line2);
    
    coverage.covered_files.push_back("src/main.cpp");
    coverage.covered_files.push_back("src/example.cpp");
    coverage.total_coverage_percentage = 80.0;
    
    return coverage;
}

} // namespace apr_system
