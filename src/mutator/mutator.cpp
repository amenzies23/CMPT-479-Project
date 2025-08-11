#include "mutator.h"
#include "../core/logger.h"

namespace apr_system
{

// Helper function to build up the diff for each patch
std::string Mutator::makeDiff(int startLine, const std::string &orig, const std::string &mod){
    // Count how many lines are in each snippet (lines = # newlines + 1)
    int origLineCount = std::count(orig.begin(), orig.end(), '\n') + 1;
    int modLineCount = std::count(mod.begin(), mod.end(), '\n') + 1;

    // Begin the header
    std::ostringstream diff;
    diff << "@@ -" << startLine << "," << origLineCount
        << " +" << startLine << "," << modLineCount << " @@\n";

    // Emit each original line prefixed with '-'
    {
        std::istringstream in(orig);
        std::string line;
        while (std::getline(in, line)){
            diff << "-" << line << "\n";
        }
    }

    // Emit each modified line prefixed with '+'
    {
        std::istringstream in(mod);
        std::string line;
        while (std::getline(in, line)){
            diff << "+" << line << "\n";
        }
    }

    return diff.str();
}

std::vector<PatchCandidate> Mutator::generatePatches(
    const std::vector<ASTNode> &ast_nodes,
    const std::vector<std::string> &source_files){
    LOG_COMPONENT_INFO("mutator", "this is a stub implementation");
    LOG_COMPONENT_INFO("mutator", "input: {} AST nodes, {} source files",
                        ast_nodes.size(), source_files.size());

    // Split up the vector of nodes into fix-ingredients and suspicious nodes
    std::vector<const ASTNode *> targets, ingredients;
    for (auto &node : ast_nodes){     
        // Building up the fix-ingredients to be ALL the nodes in the file, not just the non-suspicious nodes
        // Since supsicious nodes are all probalistic, there will be a handful of suspicious nodes that are actually valid and not broken
        // so it makes sense to include them in the overall pool of fix ingredients
        ingredients.push_back(&node);
        if (node.suspiciousness_score > 0.0){
            targets.push_back(&node);
        }
    }

    // Helpful for debugging, prints out all suspicious nodes and fix ingredients into text files in the build directory
    dumpSuspiciousNodes(targets);
    dumpFixIngredients(ingredients);

    std::vector<PatchCandidate> patch_candidates;
    int id_counter = 0;

    /**
     * For each target (suspicious node), we iterate over all source (fix‐ingredient) nodes
     * and apply each historical mutation rule in hist_ to generate PatchCandidate objects:
     *
     *   Replacement:
     *     - Look up entries in hist_.replacement whose target_node matches t->node_type.
     *     - Only consider source (fix ingredient) nodes s where s->node_type == t->node_type.
     *     - Skip any multi line replacements (Only considering single-line patches for now)
     *     - Build a diff, compute replacement similarity (genealogy × dependency × variable),
     *       record suspiciousness and similarity scores.
     *
     *   Insertion:
     *     - Look up entries in hist_.insertion matching t->node_type and s->node_type.
     *     - Skip multi‐line insertions
     *     - Construct the diff with orig="" and mod = s->source_text.
     *     - Compute insertion similarity (genealogy × dependency) and record the scores
     *
     *   Deletion:
     *     - Look up entries in hist_.deletion matching t->node_type and s->node_type.
     *     - Skip multi‐line deletions.
     *     - Construct the diff with mod="" and orig = t->source_text.
     *     - Compute deletion similarity (genealogy × dependency), record scores.
     */
    for (auto *t : targets){
        for (auto *s : ingredients){
            // Replacement
            for (auto &e : hist_.replacement){
                if (e.target_node == t->node_type && s->node_type == t->node_type){
                    auto &orig = t->source_text;
                    auto &mod  = s->source_text;
                    if (orig.find('\n') != std::string::npos || mod.find('\n')  != std::string::npos) continue; // skip multi-line edits
                    if (orig == mod) continue; // skip patches with the exact same code as the original (avoid duplicates)
                    
                    PatchCandidate p;
                    p.patch_id = "patch_" + std::to_string(id_counter++);
                    p.target_node_id   = t->node_id;
                    p.file_path = t->file_path;
                    p.start_line = t->start_line;
                    p.end_line = t->end_line;
                    p.original_code = t->source_text;
                    p.modified_code = s->source_text;
                    p.diff = makeDiff(t->start_line,
                                        p.original_code,
                                        p.modified_code);
                    p.mutation_type.mutation_category = "Replacement";
                    p.mutation_type.target_node = t->node_type;
                    p.mutation_type.source_node = s->node_type;

                    p.suspiciousness_score = t->suspiciousness_score;
                    p.similarity_score = computeReplacementSimilarity(
                        s->genealogy_context, t->genealogy_context,
                        s->dependency_context, t->dependency_context,
                        s->variable_context, t->variable_context);

                    patch_candidates.push_back(std::move(p));
                }
            }

            // Insertion
            for (auto &e : hist_.insertion){
                if (e.target_node == t->node_type && e.source_node == s->node_type){

                    auto &orig = t->source_text;
                    auto &mod  = s->source_text;
                    if (orig.find('\n') != std::string::npos || mod.find('\n')  != std::string::npos) continue;

                    PatchCandidate p;
                    p.patch_id = "patch_" + std::to_string(id_counter++);
                    p.target_node_id   = t->node_id;
                    p.file_path = t->file_path;
                    p.start_line = t->start_line;
                    p.end_line = t->start_line;
                    p.original_code = "";
                    p.modified_code = s->source_text;
                    p.diff = makeDiff(t->start_line,
                                        p.original_code,
                                        p.modified_code);

                    p.mutation_type.mutation_category = "Insertion";
                    p.mutation_type.target_node = t->node_type;
                    p.mutation_type.source_node = s->node_type;

                    p.suspiciousness_score = t->suspiciousness_score;
                    p.similarity_score = computeInsertionSimilarity(
                        s->genealogy_context, t->genealogy_context,
                        s->dependency_context, t->dependency_context);

                    patch_candidates.push_back(std::move(p));
                }
            }

            // Deletion
            for (auto &e : hist_.deletion){
                if (e.target_node == t->node_type && e.source_node == s->node_type){
                    auto &orig = t->source_text;
                    auto &mod  = s->source_text;
                    if (orig.find('\n') != std::string::npos || mod.find('\n')  != std::string::npos) continue;

                    PatchCandidate p;
                    p.patch_id = "patch_" + std::to_string(id_counter++);
                    p.target_node_id   = t->node_id;
                    p.file_path = t->file_path;
                    p.start_line = t->start_line;
                    p.end_line = t->end_line;
                    p.original_code = t->source_text;
                    p.modified_code = "";
                    p.diff = makeDiff(t->start_line,
                                        p.original_code,
                                        p.modified_code);

                    p.mutation_type.mutation_category = "Deletion";
                    p.mutation_type.target_node = t->node_type;
                    p.mutation_type.source_node = s->node_type;

                    p.suspiciousness_score = t->suspiciousness_score;
                    p.similarity_score = computeDeletionSimilarity(
                        s->genealogy_context, t->genealogy_context,
                        s->dependency_context, t->dependency_context);

                    patch_candidates.push_back(std::move(p));
                }
            }
        }
    }
    dumpPatchCandidates(patch_candidates); 

    LOG_COMPONENT_INFO("mutator", "stub returning {} mock patch candidates", patch_candidates.size());
    return patch_candidates;
}

} // namespace apr_system
