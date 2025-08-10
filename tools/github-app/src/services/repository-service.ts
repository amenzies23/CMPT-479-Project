/**
 * @file repository management service
 * @path src/services/repository-service.ts
 * 
 * handles repository cloning and file system operations for apr analysis.
 * manages temporary directories with automatic cleanup and error handling.
 * 
 * @core-features
 * - secure repository cloning with token authentication
 * - specific commit checkout for consistent analysis
 * - temporary directory management with automatic cleanup
 * - comprehensive error handling with actionable messages
 */

import { execSync } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';
import { SimpleWorkflowEvent } from '../types';

export class RepositoryService {
  private tempDir: string | null = null;

  /**
   * clone repository to temporary directory
   * creates a clone with token authentication and checks out specific commit
   * 
   * @param event - workflow event containing repository information
   * @param token - github access token for authentication
   * @returns promise resolving to path of cloned repository
   */
  async cloneRepository(event: SimpleWorkflowEvent, token: string): Promise<string> {
    console.log(`cloning repository: ${event.repo.owner}/${event.repo.name}`);
    console.log(`target sha: ${event.headSha}`);

    try {
      // step 1: create unique temporary directory
      const tempDirPrefix = `apr-clone-${event.repo.owner}-${event.repo.name}-`;
      const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), tempDirPrefix));
      this.tempDir = tempDir; // store for cleanup

      console.log(`created temporary directory: ${tempDir}`);

      // step 2: clone repository using token authentication
      const repoUrl = `https://x-access-token:${token}@github.com/${event.repo.owner}/${event.repo.name}.git`;
      const repoPath = path.join(tempDir, event.repo.name);

      console.log(`cloning repository to: ${repoPath}`);

      // clone the repository with timeout protection
      execSync(`git clone "${repoUrl}" "${repoPath}"`, {
        stdio: 'pipe',
        cwd: tempDir,
        timeout: 120000, // 120 second timeout
      });

      // step 3: checkout specific commit sha
      console.log(`checking out commit: ${event.headSha}`);
      execSync(`git checkout "${event.headSha}"`, {
        stdio: 'pipe',
        cwd: repoPath,
        timeout: 30000, // 30 second timeout
      });

      console.log(`✓ repository cloned and checked out successfully`);
      return repoPath;

    } catch (error) {
      console.error('✗ failed to clone repository:', error);

      // provide specific error messages for common issues
      if (error instanceof Error) {
        if (error.message.includes('timeout')) {
          throw new Error(`repository clone timed out. the repository may be too large or network is slow.`);
        } else if (error.message.includes('Authentication failed')) {
          throw new Error(`failed to authenticate with github. check app_id and private_key configuration.`);
        } else if (error.message.includes('not found')) {
          throw new Error(`repository ${event.repo.owner}/${event.repo.name} not found or not accessible.`);
        }
      }

      throw new Error(`failed to clone repository: ${error instanceof Error ? error.message : 'unknown error'}`);
    }
  }

  /**
   * get the temporary directory path
   * returns the current temporary directory or null if not set
   * 
   * @returns temporary directory path or null
   */
  getTempDir(): string | null {
    return this.tempDir;
  }

  /**
   * clean up temporary directory
   * removes temporary directory and all contents with retry logic
   * 
   * @performance uses recursive removal with 3 retries and 100ms delay
   */
  async cleanup(): Promise<void> {
    if (!this.tempDir) {
      return;
    }

    try {
      if (fs.existsSync(this.tempDir)) {
        fs.rmSync(this.tempDir, {
          recursive: true,
          force: true,
          maxRetries: 3,
          retryDelay: 100
        });
        console.log(`✓ cleaned up temporary directory: ${this.tempDir}`);
      }
    } catch (error) {
      console.error(`✗ failed to cleanup temporary directory: ${error}`);
    } finally {
      this.tempDir = null;
    }
  }
}