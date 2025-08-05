/**
 * @file apr results parsing service
 * @path src/services/apr-results-parser.ts
 * 
 * handles parsing of apr binary output and result interpretation.
 * extracts patches from validation results and suspicious locations with confidence scoring.
 * 
 * @core-features
 * - pipeline_results.json parsing with multiple fallback strategies
 * - patch extraction from validation results and suspicious locations
 * - confidence scoring based on compilation success and test results
 * - comprehensive error handling with actionable messages
 */

import { APRResult, Patch } from '../types';
import * as fs from 'fs';
import * as path from 'path';

export class APRResultsParser {
  /**
   * parse apr results from pipeline_results.json
   * extracts patches with confidence scoring and comprehensive error handling
   * 
   * @param resultsPath - path to the pipeline_results.json file
   * @param outputDir - apr output directory for additional files
   * @returns promise resolving to parsed apr results
   */
  async parseResults(resultsPath: string, outputDir: string): Promise<APRResult> {
    console.log(`parsing apr results from: ${resultsPath}`);

    try {
      // check if results file exists
      if (!fs.existsSync(resultsPath)) {
        console.error(`✗ results file not found: ${resultsPath}`);
        return {
          success: false,
          patches: [],
          errorMessage: 'apr results file not found'
        };
      }

      // read and parse json results
      const resultsContent = fs.readFileSync(resultsPath, 'utf-8');
      console.log('raw apr results content length:', resultsContent.length);

      const results = JSON.parse(resultsContent);
      console.log('parsed apr results structure:', {
        hasPipelineSummary: !!results.pipeline_summary,
        hasSuspiciousLocations: !!results.suspicious_locations,
        hasValidationResults: !!results.validation_results,
        hasPatchCandidates: !!results.patch_candidates,
        suspiciousLocationsCount: results.suspicious_locations?.length || 0,
        validationResultsCount: results.validation_results?.length || 0,
        patchCandidatesCount: results.patch_candidates?.length || 0
      });

      // extract patches from validation_results and suspicious_locations
      const patches = this.extractPatches(results);

      // determine overall success based on validation results
      const hasSuccessfulValidation = results.validation_results &&
        Array.isArray(results.validation_results) &&
        results.validation_results.some((result: any) =>
          result.compilation_success && result.tests_passed
        );

      const success = hasSuccessfulValidation && patches.length > 0;

      console.log(`✓ parsed apr results: ${patches.length} patches, success: ${success}`);
      console.log('patch summary:', patches.map(p => `${p.file}:${p.line} (confidence: ${p.confidence})`));

      const result: APRResult = {
        success: success,
        patches: patches
      };

      if (!success) {
        result.errorMessage = this.generateErrorMessage(results, patches.length);
      }

      return result;

    } catch (error) {
      console.error('✗ failed to parse APR results:', error);

      let errorMessage = 'failed to parse APR results';

      if (error instanceof Error) {
        if (error.message.includes('JSON') || error.message.includes('parse')) {
          errorMessage = 'invalid JSON in APR results file, APR binary may have produced malformed output. check binary execution logs for details.';
        } else if (error.message.includes('ENOENT')) {
          errorMessage = 'apr results file not found, binary execution may have failed silently or crashed.';
        } else if (error.message.includes('EACCES')) {
          errorMessage = 'permission denied reading apr results file. check file permissions.';
        } else {
          errorMessage = `apr results parsing error: ${error.message}`;
        }
      }

      console.error(`✗ results parsing failed, will not create pull request: ${errorMessage}`);

      return {
        success: false,
        patches: [],
        errorMessage: errorMessage
      };
    }
  }

  /**
   * extract patches from apr results
   * processes validation results and suspicious locations to create patch objects
   * 
   * @param results - parsed apr results json
   * @returns array of extracted patches with confidence scores
   */
  private extractPatches(results: any): Patch[] {
    const patches: Patch[] = [];

    // first, try to extract patches from validation results that have successful compilation and tests
    if (results.validation_results && Array.isArray(results.validation_results)) {
      console.log(`processing ${results.validation_results.length} validation results`);

      for (const validationResult of results.validation_results) {
        // only process patches that compiled successfully and passed tests
        if (validationResult.compilation_success && validationResult.tests_passed) {
          console.log(`✓ found successful patch: ${validationResult.patch_id}`);

          // try to find corresponding patch candidate data
          let patchCandidate = null;

          if (results.patch_candidates && Array.isArray(results.patch_candidates)) {
            patchCandidate = results.patch_candidates.find((candidate: any) =>
              candidate.patch_id === validationResult.patch_id
            );
          }

          // if we found detailed patch candidate data, use it
          if (patchCandidate) {
            patches.push({
              file: patchCandidate.file_path,
              line: patchCandidate.start_line,
              original: patchCandidate.original_code,
              fixed: patchCandidate.modified_code,
              confidence: this.calculateConfidence(validationResult, patchCandidate)
            });
            console.log(`✓ added patch from candidate data: ${patchCandidate.file_path}:${patchCandidate.start_line}`);
          } else {
            // fallback: create patch based on suspicious locations
            const suspiciousLocation = results.suspicious_locations?.find((loc: any) =>
              loc.file_path && loc.line_number
            );

            if (suspiciousLocation) {
              patches.push({
                file: suspiciousLocation.file_path,
                line: suspiciousLocation.line_number,
                original: `// original code at line ${suspiciousLocation.line_number}`,
                fixed: `// fixed code (patch ${validationResult.patch_id})`,
                confidence: suspiciousLocation.suspiciousness_score || 0.7
              });
              console.log(`✓ added patch from suspicious location: ${suspiciousLocation.file_path}:${suspiciousLocation.line_number}`);
            } else {
              // last resort: create a generic patch entry
              patches.push({
                file: 'unknown',
                line: 1,
                original: `// original code (patch ${validationResult.patch_id})`,
                fixed: `// fixed code (patch ${validationResult.patch_id})`,
                confidence: 0.6
              });
              console.log(`✓ added generic patch entry for: ${validationResult.patch_id}`);
            }
          }
        }
      }
    }

    // if no successful validation results, try to extract patches from suspicious locations
    if (patches.length === 0 && results.suspicious_locations && Array.isArray(results.suspicious_locations)) {
      console.log(`no successful validation results, processing ${results.suspicious_locations.length} suspicious locations`);

      for (const location of results.suspicious_locations) {
        if (location.file_path && location.line_number) {
          patches.push({
            file: location.file_path,
            line: location.line_number,
            original: `// suspicious code at line ${location.line_number}`,
            fixed: `// potential fix for suspicious code`,
            confidence: location.suspiciousness_score || 0.5
          });
          console.log(`✓ added patch from suspicious location: ${location.file_path}:${location.line_number}`);
        }
      }
    }

    return patches;
  }

  /**
   * TODO: implenent this, it's kinda a mock rn (for testing)
   * 
   * calculate confidence score for a patch
   * combines compilation success, test results, and mutation type factors
   * 
   * @param validationResult - validation result from apr engine
   * @param patchCandidate - optional patch candidate data
   * @returns confidence score between 0.0 and 1.0
   */
  private calculateConfidence(validationResult: any, patchCandidate?: any): number {
    let confidence = 0.5; // base confidence

    // boost confidence if compilation succeeded
    if (validationResult.compilation_success) {
      confidence += 0.2;
    }

    // boost confidence if tests passed
    if (validationResult.tests_passed) {
      confidence += 0.2;
    }

    // boost confidence based on test pass rate
    if (validationResult.tests_passed_count && validationResult.tests_total_count) {
      const passRate = validationResult.tests_passed_count / validationResult.tests_total_count;
      confidence += passRate * 0.1;
    }

    // TODO: consider patch candidate mutation type if available
    if (patchCandidate?.mutation_type) { }

    // ensure confidence is within bounds
    return Math.min(1.0, Math.max(0.0, confidence));
  }

  /**
   * generate appropriate error message based on apr results
   * provides detailed diagnostics for different failure scenarios
   * 
   * @param results - parsed apr results json
   * @param patchCount - number of patches extracted
   * @returns descriptive error message for debugging
   */
  private generateErrorMessage(results: any, patchCount: number): string {
    if (!results.validation_results || !Array.isArray(results.validation_results)) {
      return 'no validation results found in apr output. the apr engine may not have completed successfully or the output format is unexpected.';
    }

    if (results.validation_results.length === 0) {
      return 'apr engine did not generate any patches to validate. this may indicate that no suspicious locations were identified or the fault localization failed.';
    }

    const compilationFailures = results.validation_results.filter((r: any) => !r.compilation_success).length;
    const testFailures = results.validation_results.filter((r: any) => r.compilation_success && !r.tests_passed).length;
    const successfulPatches = results.validation_results.filter((r: any) => r.compilation_success && r.tests_passed).length;
    const totalPatches = results.validation_results.length;

    if (compilationFailures === totalPatches) {
      return `all ${totalPatches} generated patches failed to compile. this suggests the mutations may be too aggressive or the build environment has issues.`;
    }

    if (testFailures > 0 && patchCount === 0) {
      return `${testFailures} patches compiled successfully but failed tests, ${compilationFailures} failed to compile. the patches may not address the root cause of the test failures.`;
    }

    if (successfulPatches > 0 && patchCount === 0) {
      return `${successfulPatches} patches passed all tests but could not be extracted for pr creation. this may be a parsing issue with the patch candidate data.`;
    }

    const summary = `apr analysis completed but no viable patches were found. ` +
      `results: ${successfulPatches} successful, ${testFailures} test failures, ${compilationFailures} compilation failures out of ${totalPatches} total attempts.`;

    if (totalPatches < 5) {
      return `${summary} consider running apr with different parameters or checking if the failing tests are deterministic.`;
    }

    return `${summary} the fault may be too complex for automated repair or require domain-specific knowledge.`;
  }

}