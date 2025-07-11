#pragma once

#include "../core/contracts.h"
#include <string>
#include <vector>

namespace apr_system {

/**
 * @brief stub implementation of AST parser
 *
 * this is a stub to implement AST parsing.
 * currently returns mock data for testing the data flow between modules.
 */
class Parser : public IParser {
public:
  Parser() = default;
  ~Parser() = default;

  /**
   * @brief stub method for AST parsing
   *
   * @param suspicious_locations locations identified by SBFL with file_path and
   * line_number
   * @param source_files paths to source files that should be parsed
   * @return vector of AST nodes that represent syntactic elements that can be
   * mutated
   */
  std::vector<ASTNode>
  parseAST(const std::vector<SuspiciousLocation> &suspicious_locations,
           const std::vector<std::string> &source_files) override;
};

} // namespace apr_system
