#pragma once

#include "../core/contracts.h"
#include <string>
#include <vector>

namespace apr_system {

/**
 * @brief stub implementation of mutator (patch generator)
 *
 * this is a stub to implement patch generation.
 * currently returns mock data for testing the data flow between modules.
 */
class Mutator : public IMutator {
public:
  Mutator() = default;
  ~Mutator() = default;

  /**
   * @brief stub method for patch generation
   *
   * @param ast_nodes AST nodes from parser representing code that can be
   * mutated
   * @param source_files original source file paths for context
   * @return vector of patch candidates with mutation details
   */
  std::vector<PatchCandidate>
  generatePatches(const std::vector<ASTNode> &ast_nodes,
                  const std::vector<std::string> &source_files) override;
};

} // namespace apr_system
