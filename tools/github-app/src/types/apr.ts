/**
 * @file APR-specific type definitions
 * @path src/types/apr.ts
 * 
 * @overview
 * typescript type definitions for APR engine integration.
 * defines configuration, results, and data structures for automated program repair.
 */

import { RepoIdentifier } from "./events";

/**
 * configuration for APR engine invocation
 */
export interface APRConfig {
  repoPath: string;              // local path to cloned repository
  repoUrl: string;               // github repository url
  branch: string;                // target branch name
  commitSha: string;             // commit sha to analyze
  testResultsPath: string;       // path to test_results.json
  coveragePath: string;          // path to coverage.xml
  outputDir: string;             // directory for APR engine output
  maxPatches: number;            // maximum patches to generate
  confidenceThreshold: number;   // minimum confidence threshold
}

/**
 * project detection result
 */
export interface ProjectConfig {
  type: "cmake" | "unknown";     // detected project type
  buildScript: string;           // build command
  testScript: string;            // test command
  sourceFiles: string[];         // detected source files
  configFile?: string;           // path to .apr.yml if exists
}

/**
 * test result from CI artifacts
 */
export interface TestResult {
  name: string;                  // test name
  status: "PASSED" | "FAILED";   // test status
  execution_time_ms: number;     // execution time
  error_message?: string;        // error message if failed
  file?: string;                 // test file path
  line?: number;                 // test line number
}

/**
 * test results summary from CI
 */
export interface TestResultsSummary {
  summary: {
    total_tests: number;
    passed_tests: number;
    failed_tests: number;
    execution_time_ms: number;
  };
  tests: TestResult[];
}

/**
 * patch candidate from APR engine
 */
export interface PatchCandidate {
  patch_id: string;              // unique patch identifier
  file_path: string;             // target file path
  start_line: number;            // start line of patch
  end_line: number;              // end line of patch
  original_code: string;         // original code
  modified_code: string;         // modified code
  diff: string;                  // unified diff
  mutation_type: string;         // type of mutation applied
  affected_tests: string[];      // tests affected by this patch
}

/**
 * validation result for a patch
 */
export interface ValidationResult {
  patch_id: string;              // patch identifier
  compilation_success: boolean;  // whether patch compiles
  tests_passed: boolean;         // whether tests pass with patch
  build_time_ms: number;         // build time
  test_time_ms: number;          // test execution time
  build_output: string;          // build output/errors
  test_output: string;           // test output
  error_message: string;         // error message if failed
  tests_passed_count: number;    // number of tests that passed
  tests_total_count: number;     // total number of tests
}

/**
 * APR engine execution result
 */
export interface APRResult {
  success: boolean;              // whether APR engine succeeded
  pipelineResults: any;          // raw JSON output from APR engine
  patches: PatchCandidate[];     // generated patch candidates
  validationResults: ValidationResult[]; // validation results
  errorMessage?: string;         // error message if failed
  engineVersion?: string;        // version of APR engine that was used
}

/**
 * repository cloning result
 */
export interface CloneResult {
  success: boolean;              // whether cloning succeeded
  repoPath: string;              // local path to cloned repo
  errorMessage?: string;         // error message if failed
}