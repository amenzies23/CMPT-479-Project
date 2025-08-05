/**
 * @file minimal event type definitions
 * @path src/types/events.ts
 * 
 * @overview
 * minimal type definitions for the github APR bot
 * only includes types needed for workflow failure handling
 */

/**
 * identifies a github repository
 */
export interface RepoIdentifier {
  owner: string;  // repository owner username
  name: string;   // repository name
}

/**
 * domain event for workflow run completion
 * triggered when CI workflows complete (filtered to failures)
 */
export interface WorkflowRunEvent {
  installationId: number;        // github app installation id
  repo: RepoIdentifier;          // target repository
  workflowName: string;          // name of the failed workflow
  conclusion: string;            // workflow conclusion (failure, success)
  headSha: string;               // commit sha that triggered the workflow
  headBranch: string;            // branch that triggered the workflow
  runId: number;                 // workflow run id for artifact access
  runUrl: string;                // workflow run url for reference
  sender: string;                // event sender username
} 