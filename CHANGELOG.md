# CHANGELOG

all notable changes to the project will be documented in this file.

the format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### ADDED - 2025-07-08
- initial project skeleton with core APR pipeline modules
- module interfaces and stub implementations (SBFL, parser, mutator, prioritizer, validator, PRBot)
- pipeline orchestrator for coordinating module execution
- cmake build system with proper dependency management
- logging infrastructure with component-specific loggers (using spdlog)
- google Test framework integration with unit and integration tests
- JSON schema validation framework (temporarily disabled)
- core data structures and contracts for inter-module communication
