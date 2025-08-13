#include "utils.h"
#include <fstream>

namespace apr_system {

void dumpSuspiciousLocations(const std::vector<SuspiciousLocation>& locations) {
    std::ofstream out("SuspiciousLocations.txt");
    if (!out) return;
    for (const auto& loc : locations) {
        out << "file_path: " << loc.file_path << "\n"
            << "line_number: " << loc.line_number << "\n"
            << "suspiciousness_score: " << loc.suspiciousness_score << "\n";
    }
}
}