#pragma once
#include "mutator.h"
#include <vector>

namespace apr_system {
  void dumpSuspiciousNodes(const std::vector<const ASTNode*>& targets);
  void dumpPatchCandidates(const std::vector<PatchCandidate>& patches);
  void dumpFixIngredients(const std::vector<const ASTNode*>& ingredients);
}