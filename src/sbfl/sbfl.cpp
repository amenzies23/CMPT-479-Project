#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include "sbfl.h"
#include "../core/logger.h"
#include "utils.h"
#include <iostream>

namespace apr_system {


void SBFL::runSBFLAnalysis(const std::string& buggy_program_dir, std::string& sbfl_json) {
    std::string coverage_dir = buggy_program_dir + "/build/coverage";
    std::string results_json = buggy_program_dir + "/build/coverage/results.json";
    sbfl_json = coverage_dir + "/sbfl_results.json";

    std::string cmd = "/.venv/bin/python /workspace/src/sbfl/sbfl_analysis.py " + results_json + " " + coverage_dir;
    LOG_COMPONENT_INFO("running SBFL analysis: {}", cmd);

    if (std::system(cmd.c_str()) == 0) {
        LOG_COMPONENT_INFO("SBFL results generated at: {}", sbfl_json);
    } else {
        LOG_COMPONENT_ERROR("sbfl", "SBFL analysis script failed to execute");
    }
}

std::vector<SuspiciousLocation> SBFL::localizeFaults(const std::string& sbfl_json) {
    std::vector<SuspiciousLocation> locations;
    LOG_COMPONENT_INFO("sbfl", "parsing JSON results from: {}", sbfl_json);

    try {
        std::ifstream file(sbfl_json);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open output file: " + sbfl_json);
        }

        nlohmann::json data = nlohmann::json::parse(file);

        if (data.contains("data")) {
            const auto& data_array = data["data"];
            LOG_COMPONENT_INFO("sbfl", "found {} entries in data array", data_array.size());

            for (const auto& item : data_array) {
                SuspiciousLocation location;
                std::string full_path = item.value("file", "unknown");

                // extract the relative path
                std::string marker = "/workspace/buggy-programs/";
                size_t pos = full_path.find(marker);
                if (pos != std::string::npos) {
                    location.file_path = full_path.substr(pos);
                } else {
                    continue;
                }

                location.line_number = item.value("line", 0);
                location.suspiciousness_score = item.value("score", 0.0);
                locations.push_back(location);
            }

            // sort by suspicious score (descending)
            std::sort(locations.begin(), locations.end(), 
                [](const SuspiciousLocation& a, const SuspiciousLocation& b) {
                    return a.suspiciousness_score > b.suspiciousness_score;
                });
            
            // dumpSuspiciousLocations(locations);
        }
    } catch (const std::exception& e) {
        LOG_COMPONENT_ERROR("sbfl", "error parsing JSON results: {}", e.what());
    }

    return locations;
}

} // namespace apr_system
