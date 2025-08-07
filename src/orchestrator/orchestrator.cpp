#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fmt/core.h>

#include "orchestrator.h"
#include "../core/logger.h"

namespace apr_system {

Orchestrator::Orchestrator() {
    LOG_COMPONENT_INIT("orchestrator");

    // TEMPORARILY DISABLED - JSON schema validator
    /*
    // determine correct schema path based on current working directory
    std::string schema_path = "config/schemas";
    if (std::filesystem::exists("../config/schemas")) {
        schema_path = "../config/schemas";
    }

    // initialize schema validator
    schema_validator_ = std::make_unique<JSONSchemaValidator>(schema_path);
    if (!schema_validator_->initialize()) {
        LOG_WARN("failed to initialize JSON schema validator");
    } else {
        LOG_DEBUG("JSON schema validator initialized successfully");
    }
    */
}

SystemState Orchestrator::runPipeline(
    const RepositoryMetadata& repo_metadata,
    const std::string& sbfl_json,
    const std::string& mutation_freq_json
) {
    std::vector<TestResult> test_results;
    validateComponents();

    SystemState state{
        .repo_metadata = repo_metadata,
        .suspicious_locations = {},
        .ast_nodes = {},
        .patch_candidates = {},
        .prioritized_patches = {},
        .validation_results = {},
        .has_pr_result = false,
        .pr_result = {}
    };

    // step 1: fault localization
    LOG_COMPONENT_INFO("sbfl", "running fault localization...");
    state.suspicious_locations = sbfl_->localizeFaults(sbfl_json);
    LOG_COMPONENT_INFO("sbfl", "fault localization completed - found {} suspicious locations", state.suspicious_locations.size());

    if (state.suspicious_locations.empty()) {
        LOG_COMPONENT_WARN("sbfl", "no suspicious locations found, stopping pipeline");
        return state;
    }

    // step 2: AST parsing
    LOG_COMPONENT_INFO("parser", "parsing source files...");
    state.ast_nodes = parser_->parseAST(state.suspicious_locations, repo_metadata.source_files);
    LOG_COMPONENT_INFO("parser", "AST parsing completed - extracted {} AST nodes", state.ast_nodes.size());

    if (state.ast_nodes.empty()) {
        LOG_COMPONENT_WARN("parser", "no AST nodes found, stopping pipeline");
        return state;
    }

    // step 3: patch generation
    LOG_COMPONENT_INFO("mutator", "generating patch candidates...");
    state.patch_candidates = mutator_->generatePatches(state.ast_nodes, repo_metadata.source_files);
    LOG_COMPONENT_INFO("mutator", "patch generation completed - generated {} patch candidates", state.patch_candidates.size());

    if (state.patch_candidates.empty()) {
        LOG_COMPONENT_WARN("mutator", "no patch candidates generated, stopping pipeline");
        return state;
    }

    // step 4: patch prioritization
    LOG_COMPONENT_INFO("prioritizer", "prioritizing patches...");
    state.prioritized_patches = prioritizer_->prioritizePatches(state.patch_candidates, mutation_freq_json);
    LOG_COMPONENT_INFO("prioritizer", "patch prioritization completed - prioritized {} patches", state.prioritized_patches.size());

    if (state.prioritized_patches.empty()) {
        LOG_COMPONENT_WARN("prioritizer", "no patches prioritized, stopping pipeline");
        return state;
    }

    // step 5: patch validation
    LOG_COMPONENT_INFO("validator", "validating patches...");
    state.validation_results = validator_->validatePatches(state.prioritized_patches, repo_metadata, 3);
    LOG_COMPONENT_INFO("validator", "patch validation completed - validated {} patches", state.validation_results.size());

    if (state.validation_results.empty()) {
        LOG_COMPONENT_WARN("validator", "no patches validated successfully, stopping pipeline");
        return state;
    }

    // step 6: find best patch
    auto best_patch_it = std::max_element(state.validation_results.begin(),
                                         state.validation_results.end(),
        [](const ValidationResult& a, const ValidationResult& b) {
            return a.tests_passed_count < b.tests_passed_count;
        });

    if (best_patch_it != state.validation_results.end() && best_patch_it->tests_passed) {
        // step 7: create pull request
        LOG_COMPONENT_INFO("prbot", "creating pull request...");
        state.pr_result = prbot_->createPullRequest(*best_patch_it, repo_metadata, state.validation_results);
        state.has_pr_result = true;

        if (state.pr_result.success) {
            LOG_COMPONENT_INFO("prbot", "pull request created successfully: {}", state.pr_result.pr_url);
        } else {
            LOG_COMPONENT_ERROR("prbot", "pull request creation failed: {}", state.pr_result.pr_url);
        }
    }

    LOG_COMPONENT_INFO("orchestrator", "APR project pipeline completed successfully!");
    return state;
}

void Orchestrator::setComponents(
    std::unique_ptr<ISBFL> sbfl,
    std::unique_ptr<IParser> parser,
    std::unique_ptr<IMutator> mutator,
    std::unique_ptr<IPrioritizer> prioritizer,
    std::unique_ptr<IValidator> validator,
    std::unique_ptr<IPRBot> prbot
) {
    sbfl_ = std::move(sbfl);
    parser_ = std::move(parser);
    mutator_ = std::move(mutator);
    prioritizer_ = std::move(prioritizer);
    validator_ = std::move(validator);
    prbot_ = std::move(prbot);
}

void Orchestrator::validateComponents() const {
    if (!sbfl_) {
        LOG_APR_ERROR("orchestrator", "SBFL component not set");
        throw std::runtime_error("sbfl component not set");
    }
    if (!parser_) {
        LOG_APR_ERROR("orchestrator", "parser component not set");
        throw std::runtime_error("parser component not set");
    }
    if (!mutator_) {
        LOG_APR_ERROR("orchestrator", "mutator component not set");
        throw std::runtime_error("mutator component not set");
    }
    if (!prioritizer_) {
        LOG_APR_ERROR("orchestrator", "prioritizer component not set");
        throw std::runtime_error("prioritizer component not set");
    }
    if (!validator_) {
        LOG_APR_ERROR("orchestrator", "validator component not set");
        throw std::runtime_error("validator component not set");
    }
    if (!prbot_) {
        LOG_APR_ERROR("orchestrator", "PR bot component not set");
        throw std::runtime_error("prbot component not set");
    }

    LOG_DEBUG("all orchestrator components validated successfully");
}

void Orchestrator::logPipelineStep(const std::string& step_name, const std::string& details) const {
    if (details.empty()) {
        LOG_COMPONENT_INFO("orchestrator", "pipeline step: {}", step_name);
    } else {
        LOG_COMPONENT_INFO("orchestrator", "pipeline step: {} - {}", step_name, details);
    }
}

} // namespace apr_system
