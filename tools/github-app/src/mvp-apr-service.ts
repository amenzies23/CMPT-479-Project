/**
 * @file core apr service for automated program repair workflow
 * @path src/mvp-apr-service.ts
 *
 * orchestrates the complete apr workflow from ci failure detection to pr creation.
 * implements a linear process: authentication -> clone -> analysis -> patch -> cleanup.
 * modular architecture with focused services.
 */

import { SimpleWorkflowEvent } from './types';
import { APROrchestrator } from './services/apr-orchestrator';

/**
 * core service for automated program repair workflow orchestration
 * 
 * @workflow-steps
 * 1. authenticate with github app installation
 * 2. clone repository to temporary directory
 * 3. execute apr binary and parse results
 * 4. create pull request with generated patches
 * 5. cleanup all temporary resources
 */
export class MVPAPRService {
  private orchestrator: APROrchestrator;

  constructor() {
    this.orchestrator = new APROrchestrator();
  }

  /**
   * main entry point for processing failed workflow events
   * 
   * @param event - simplified workflow event from github webhook
   * @returns promise that resolves when processing is complete
   * 
   * delegates to the orchestrator for the complete workflow execution.
   */
  async processFailedWorkflow(event: SimpleWorkflowEvent): Promise<void> {
    return this.orchestrator.processFailedWorkflow(event);
  }
}