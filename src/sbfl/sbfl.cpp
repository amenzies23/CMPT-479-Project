#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include "sbfl.h"
#include "../core/logger.h"

namespace apr_system {

std::vector<SuspiciousLocation> SBFL::localizeFaults(const std::string& sbfl_json) {
    std::vector<SuspiciousLocation> locations;
    LOG_COMPONENT_INFO("sbfl", "parsing JSON results from: {}", sbfl_json);

    /*
     * TODO: implement fault localization algorithm here
     *
     * input contracts:
     * - sbfl_json: sbfl json result file path 
     *
     * output contract:
     * - return SuspiciousLocation objects sorted by suspiciousness score (descending)
     * - each location must have valid file_path, line_number, and score 0.0-1.0
     */

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
                location.file_path = item.value("file", "unknown");
                location.function = item.value("function", "unknown");
                location.line_number = item.value("line", 0);
                location.suspiciousness_score = item.value("score", 0.0);
                
                locations.push_back(location);
            }

            // sort by suspicious score (descending)
            std::sort(locations.begin(), locations.end(), 
                [](const SuspiciousLocation& a, const SuspiciousLocation& b) {
                    return a.suspiciousness_score > b.suspiciousness_score;
                });
        }
    } catch (const std::exception& e) {
        LOG_COMPONENT_ERROR("sbfl", "error parsing JSON results: {}", e.what());
    }

    return locations;
}

} // namespace apr_system
