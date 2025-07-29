#pragma once

#include <tree_sitter/api.h>
#include <unordered_map>
#include <string>
#include "../core/types.h"

namespace apr_system {

// Walk ancestors (skip "block") up to method_definition
TypeCountMap extractAncestorTypes(TSNode node);

// Collect siblings (named children) in nearest "block" parent
TypeCountMap extractSiblingTypes(TSNode node);

GenealogyContext extractGenealogyContext(TSNode node);

VariableContext extractVariableContext(TSNode node, const std::string &source_content);

DependencyContext backwardSlice(TSNode target,
                                TSNode root,
                                const std::string &source_content);

DependencyContext forwardSlice(TSNode target,
                               TSNode root,
                               const std::string &source_content);

DependencyContext extractDependencyContext(TSNode target,
                                           TSNode root,
                                           const std::string &source_content);

}