#pragma once

#include "../core/contracts.h"
#include <string>
#include <vector>
#include <cstdio>
#include "context.h"
#include "freq_loader.h"
#include "utils.h" 

namespace apr_system {

/**
 * @brief implementation of mutator (patch generator)
 *
 * Generates real patch candidates by matching suspicious AST nodes
 * against available ingredients and historical mutation frequencies.
 */
class Mutator : public IMutator {
  HistoricalFreqs hist_;
public:
  explicit Mutator(const std::string &frequency_json_path)
      : hist_( loadHistoricalFrequencies(frequency_json_path) ) {}
  Mutator() : hist_() {}
  ~Mutator() = default;

  /**
   * @brief generate patch candidates from AST and source files
   *
   * For each AST node flagged as suspicious, considers all non‐suspicious
   * ingredients, applies historical rules, computes diffs, similarity
   * and priority scores, and returns a vector of PatchCandidate.
   *
   * @param ast_nodes AST nodes extracted by the parser
   * @param source_files the list of source file paths for context
   * @return a sorted list of PatchCandidate objects
   */
  std::vector<PatchCandidate>
  generatePatches(const std::vector<ASTNode> &ast_nodes,
                  const std::vector<std::string> &source_files) override;

  static std::string makeDiff(int startLine,
                            const std::string &orig,
                            const std::string &mod);
};

} // namespace apr_system
