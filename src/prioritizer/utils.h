#pragma once

#include "prioritizer.h"
#include <vector>
#include <unordered_map>

namespace apr_system {

// Console debug dump of the frequency map
void printFreqMap(
  const std::unordered_map<std::string, std::vector<FreqEntry>>& freqMap);

// Console debug dump of PrioritizedPatch objects
void printPrioritizedPatches(const std::vector<PrioritizedPatch>& patches);

// File dump of PatchCandidate list to "Prioritized_Patches.txt"
void dumpPrioritizedPatchesToFile(const std::vector<PatchCandidate>& patches);

} // namespace apr_system