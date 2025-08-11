// src/mutator/freq_loader.cpp
#include "freq_loader.h"
#include <fstream>
#include <nlohmann/json.hpp>

/**
 * Loads historical mutation frequency data for the mutator component and prioritizer.
 * Currently, this data is being mocked as a placeholder to prove the concept. This will ideally
 * load in real data of historical C++ patches in the future.
 */
namespace apr_system {
    HistoricalFreqs loadHistoricalFrequencies(const std::string &path) {
    std::ifstream in(path);
    nlohmann::json J;
    in >> J;
    HistoricalFreqs H;

    // Replacement: JSON entries only have "target" & "freq"
    for (auto &item : J["Replacement"]) {
        FreqEntry e;
        e.target_node = item.at("target").get<std::string>();
        e.freq = item.at("freq").get<double>();
        e.source_node = ""; 
        H.replacement.push_back(std::move(e));
    }

    // Insertion: JSON entries have "target","source","freq"
    for (auto &item : J["Insertion"]) {
        FreqEntry e;
        e.target_node = item.at("target").get<std::string>();
        e.source_node = item.at("source").get<std::string>();
        e.freq   = item.at("freq").get<double>();
        H.insertion.push_back(std::move(e));
    }

    // Deletion: same as insertion
    for (auto &item : J["Deletion"]) {
        FreqEntry e;
        e.target_node = item.at("target").get<std::string>();
        e.source_node = item.at("source").get<std::string>();
        e.freq   = item.at("freq").get<double>();
        H.deletion.push_back(std::move(e));
    }

    return H;
    }
}
