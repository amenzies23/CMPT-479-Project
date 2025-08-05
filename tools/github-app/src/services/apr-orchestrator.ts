/**
 * @file apr workflow orchestrator service
 * @path src/services/apr-orchestrator.ts
 * 
 * main orchestrator for the automated program repair workflow.
 * coordinates authentication, repository cloning, apr execution, and pull request creation.
 * 
 * @performance
 * - bottlenecks: apr binary execution and repository cloning
 * 
 * @core-features
 * - linear workflow with proper error handling and cleanup
 * - service coordination with dependency injection pattern
 */

import { SimpleWorkflowEvent } from '../types';
import { AuthService } from './auth-service';
import { RepositoryService } from './repository-service';
import { APRBinaryService } from './apr-binary-service';
import { PullRequestService } from './pull-request-service';
import { getConfig } from '../utils/config';
import { CleanupManager } from '../utils/cleanup-manager';

/**
 * orchestrates the complete automated program repair workflow
 * coordinates all services in a linear pipeline with proper error handling
 */
export class APROrchestrator {
  private authService: AuthService;
  private repositoryService: RepositoryService;
  private aprBinaryService: APRBinaryService;
  private cleanupManager: CleanupManager;

  constructor() {
    this.authService = new AuthService();
    this.repositoryService = new RepositoryService();
    this.aprBinaryService = new APRBinaryService();
    this.cleanupManager = new CleanupManager();

    // register cleanup callbacks for all services
    this.cleanupManager.registerCleanup(() => this.repositoryService.cleanup());
    this.cleanupManager.registerCleanup(() => this.aprBinaryService.cleanup());

    // register cleanup handlers for process termination signals
    this.cleanupManager.registerCleanupHandlers();
  }

  /**
   * main entry point for processing failed workflow events
   * executes the complete apr pipeline from authentication to pull request creation
   * 
   * @param event - simplified workflow event from github webhook
   */
  async processFailedWorkflow(event: SimpleWorkflowEvent): Promise<void> {
    console.log(`processing failed workflow for ${event.repo.owner}/${event.repo.name}`);
    console.log(`workflow: ${event.workflowName}, sha: ${event.headSha}`);

    try {
      // validate environment configuration before proceeding
      getConfig(); // this will validate environment and throw if invalid

      // step 1: get installation token for github api access
      const token = await this.authService.getInstallationToken(event.installationId);
      console.log('✓ installation token obtained');

      // step 2: clone repository to temporary directory
      const repoPath = await this.repositoryService.cloneRepository(event, token);
      console.log(`✓ repository cloned to: ${repoPath}`);

      // step 3: execute apr binary on the cloned repository
      const tempDir = this.repositoryService.getTempDir();
      if (!tempDir) {
        throw new Error('temporary directory not available for apr execution');
      }

      const aprResult = await this.aprBinaryService.executeAPRBinary(event, repoPath, tempDir);
      console.log(`✓ apr analysis completed: ${aprResult.success ? 'success' : 'failed'}`);

      // step 4: create pull request with generated patches (only if apr succeeded)
      if (aprResult.success && aprResult.patches.length > 0) {
        const pullRequestService = new PullRequestService(token);
        await pullRequestService.createPullRequest(event, aprResult);
        console.log('✓ pull request created successfully');
      } else {
        console.log('⚠ no patches generated or apr failed - skipping pr creation');
        if (aprResult.errorMessage) {
          console.log(`apr error: ${aprResult.errorMessage}`);
        }
      }

    } catch (error) {
      console.error('✗ failed to process workflow:', error);

      // log detailed error information for debugging
      if (error instanceof Error) {
        console.error(`error message: ${error.message}`);
        console.error(`error stack: ${error.stack}`);
      }

      // re-throw to ensure calling code knows about the failure
      throw error;

    } finally {
      // always clean up temporary files and directories
      await this.cleanupManager.executeCleanup();
      console.log('✓ cleanup completed');
    }
  }
}