/**
 * @file apr binary execution service
 * @path src/services/apr-binary-service.ts
 * 
 * handles apr binary discovery, download, and execution with comprehensive error handling.
 * manages binary lifecycle from discovery through cleanup with timeout protection.
 * 
 * @core-features
 * - comprehensive error handling with actionable messages
 * - automatic cleanup of downloaded files and temporary data
 */

import { execAsync } from '../utils/exec-utils';
import { APRResult } from '../types';
import { SimpleWorkflowEvent } from '../types';
import * as fs from 'fs';
import * as path from 'path';

export class APRBinaryService {
  private downloadedFiles: string[] = [];

  /**
   * execute apr binary on the cloned repository
   * handles complete execution pipeline from binary discovery to result parsing
   * 
   * @param event - workflow event containing repository information
   * @param repoPath - path to cloned repository
   * @param tempDir - temporary directory for output
   * @returns promise resolving to apr analysis results
   */
  async executeAPRBinary(event: SimpleWorkflowEvent, repoPath: string, tempDir: string): Promise<APRResult> {
    console.log(`executing apr binary for repository at: ${repoPath}`);
    
    let command = 'n/a';
    
    try {
      // step 1: find apr binary in expected locations or download
      const binaryPath = await this.findAPRBinary();
      console.log(`✓ found apr binary at: ${binaryPath}`);
      
      // step 2: create output directory for apr results
      const outputDir = path.join(tempDir, 'apr-output');
      fs.mkdirSync(outputDir, { recursive: true });
      console.log(`✓ created output directory: ${outputDir}`);
      
      // step 3: execute apr binary with proper parameters
      const repoUrl = `https://github.com/${event.repo.owner}/${event.repo.name}`;
      command = `"${binaryPath}" --repo-url "${repoUrl}" --output-dir "${outputDir}" --verbose`;
      
      console.log(`executing command: ${command}`);
      console.log(`working directory: ${repoPath}`);
      
      // execute with timeout and proper error handling
      const { stdout, stderr } = await execAsync(command, {
        cwd: repoPath,
        timeout: 300000,              // 5 minute timeout
        maxBuffer: 1024 * 1024 * 10,  // 10mb buffer
      });
      
      console.log('apr binary stdout:', stdout);
      if (stderr) {
        console.log('apr binary stderr:', stderr);
      }
      
      // step 4: parse results from pipeline_results.json
      const resultsPath = path.join(outputDir, 'pipeline_results.json');
      const aprResult = await this.parseAPRResults(resultsPath, outputDir);
      
      console.log(`✓ apr execution completed successfully with ${aprResult.patches.length} patches`);
      return aprResult;
      
    } catch (error) {
      console.error('✗ apr binary execution failed:', error);
      
      let errorMessage = 'unknown apr execution error';
      
      if (error instanceof Error) {
        if (error.message.includes('timeout')) {
          errorMessage = 'apr analysis timed out after 5 minutes. the repository may be too large or complex for automated analysis.';
        } else if (error.message.includes('ENOENT')) {
          errorMessage = 'apr binary not found or not executable. ensure the binary is properly deployed and has execute permissions.';
        } else if (error.message.includes('EACCES')) {
          errorMessage = 'permission denied executing apr binary. check file permissions and execution rights.';
        } else if (error.message.includes('Command failed')) {
          const exitCodeMatch = error.message.match(/exit code (\d+)/);
          const exitCode = exitCodeMatch ? exitCodeMatch[1] : 'unknown';
          errorMessage = `apr binary execution failed with exit code ${exitCode}. check repository structure and build requirements.`;
        } else {
          errorMessage = `apr binary execution error: ${error.message}`;
        }
      }
      
      console.error(`✗ apr execution failed. will not create pull request: ${errorMessage}`);
      
      return {
        success: false,
        patches: [],
        errorMessage: errorMessage
      };
    }
  }

  /**
   * find apr binary in expected locations + fallback download
   * searches multiple paths and attempts github release download if not found locally
   * 
   * @returns promise resolving to apr binary path
   * @performance checks local paths first before network download
   */
  private async findAPRBinary(): Promise<string> {
    console.log('searching for apr binary...');
    
    const potentialPaths = [
      '/app/bin/apr_system',           // production (heroku)
      './bin/apr_system',              // local deployment
      path.join(process.cwd(), 'bin', 'apr_system'), // current working directory
    ];
    
    // check each potential path for existing binary
    for (const binaryPath of potentialPaths) {
      console.log(`checking for apr binary at: ${binaryPath}`);
      
      if (fs.existsSync(binaryPath)) {
        try {
          fs.accessSync(binaryPath, fs.constants.X_OK);
          console.log(`✓ found executable apr binary at: ${binaryPath}`);
          return binaryPath;
        } catch (error) {
          console.log(`⚠ binary found but not executable at: ${binaryPath}`);
          try {
            fs.chmodSync(binaryPath, 0o755);
            console.log(`✓ made binary executable: ${binaryPath}`);
            return binaryPath;
          } catch (chmodError) {
            console.log(`✗ failed to make binary executable: ${chmodError}`);
          }
        }
      }
    }
    
    // if no binary found locally, try to download from github release
    console.log('no local apr binary found, attempting to download from github release...');
    
    try {
      return await this.downloadAPRBinary();
    } catch (downloadError) {
      const errorMessage = `apr binary not found and download failed. ` +
        `checked paths: ${potentialPaths.join(', ')}. ` +
        `download error: ${downloadError instanceof Error ? downloadError.message : 'unknown error'}. ` +
        `please ensure the apr binary is available at one of the expected locations or check network connectivity.`;
      
      console.error(`✗ ${errorMessage}`);
      throw new Error(errorMessage);
    }
  }

  /**
   * download apr binary from github release
   * handles platform detection, download, extraction, and cleanup
   * 
   * @returns promise resolving to downloaded binary path
   */
  private async downloadAPRBinary(): Promise<string> {
    const releaseUrl = 'https://github.com/arusinova/apr-project/releases/download/v1.0.4';
    
    let binaryUrl: string;
    let archiveName: string;
    
    // determine platform-specific download url
    if (process.platform === 'linux') {
      binaryUrl = `${releaseUrl}/apr-system-linux-x64.tar.gz`;
      archiveName = 'apr-system-linux-x64.tar.gz';
    } else {
      throw new Error(`unsupported platform: ${process.platform}. apr binary is only available for linux.`);
    }
    
    console.log(`downloading apr binary from: ${binaryUrl}`);
    
    const binDir = path.join(process.cwd(), 'bin');
    
    try {
      fs.mkdirSync(binDir, { recursive: true });
      
      const archivePath = path.join(binDir, archiveName);
      this.downloadedFiles.push(archivePath);
      
      console.log(`downloading to: ${archivePath}`);
      await execAsync(`curl -L -o "${archivePath}" "${binaryUrl}"`, {
        timeout: 120000, // 2 minute timeout for download
      });
      
      console.log('extracting archive...');
      await execAsync(`tar -xzf "${archivePath}" -C "${binDir}"`, {
        timeout: 30000, // 30 second timeout for extraction
      });
      
      // clean up archive immediately after extraction
      try {
        fs.unlinkSync(archivePath);
        this.downloadedFiles = this.downloadedFiles.filter(f => f !== archivePath);
        console.log(`✓ cleaned up archive file: ${archivePath}`);
      } catch (unlinkError) {
        console.warn(`warning: could not clean up archive file ${archivePath}:`, unlinkError);
      }
      
      const binaryPath = path.join(binDir, 'apr_system');
      fs.chmodSync(binaryPath, 0o755);
      
      console.log(`✓ successfully downloaded and extracted apr binary to: ${binaryPath}`);
      return binaryPath;
      
    } catch (error) {
      console.error('✗ failed to download apr binary:', error);
      
      const archivePath = path.join(binDir, archiveName);
      if (fs.existsSync(archivePath)) {
        try {
          fs.unlinkSync(archivePath);
          this.downloadedFiles = this.downloadedFiles.filter(f => f !== archivePath);
        } catch (cleanupError) {
          console.warn(`warning: could not clean up partial download ${archivePath}:`, cleanupError);
        }
      }
      
      throw new Error(`failed to download apr binary from github release: ${error instanceof Error ? error.message : 'unknown error'}`);
    }
  }

  /**
   * parse apr results from pipeline_results.json
   * delegates to apr results parser service for detailed parsing logic
   * 
   * @param resultsPath - path to pipeline_results.json file
   * @param outputDir - apr output directory for additional files
   * @returns promise resolving to parsed apr results
   */
  private async parseAPRResults(resultsPath: string, outputDir: string): Promise<APRResult> {
    const { APRResultsParser } = await import('./apr-results-parser');
    const parser = new APRResultsParser();
    return parser.parseResults(resultsPath, outputDir);
  }

  /**
   * clean up downloaded files and temporary data
   * removes all files tracked during download and extraction process
   * 
   * @performance iterates through tracked files for cleanup
   */
  async cleanup(): Promise<void> {
    for (const filePath of this.downloadedFiles) {
      try {
        if (fs.existsSync(filePath)) {
          fs.unlinkSync(filePath);
          console.log(`✓ cleaned up downloaded file: ${filePath}`);
        }
      } catch (error) {
        console.error(`✗ failed to cleanup downloaded file ${filePath}:`, error);
      }
    }
    this.downloadedFiles = [];
  }
}