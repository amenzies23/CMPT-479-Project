#pragma once
#include "../core/types.h"
#include <string>
#include <vector>

namespace apr_system {
  struct HistoricalFreqs {
    std::vector<FreqEntry> replacement;
    std::vector<FreqEntry> insertion;
    std::vector<FreqEntry> deletion;
  };

  HistoricalFreqs loadHistoricalFrequencies(const std::string &path);
}