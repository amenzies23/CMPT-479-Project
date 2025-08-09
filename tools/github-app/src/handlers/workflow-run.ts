/**
 * @file github workflow failure event handler
 * @path src/handlers/workflow-run.ts
 * 
 * processes github workflow_run events to detect ci failures and trigger apr analysis.
 * filters events for relevant workflows and initiates automated repair process.
 * 
 * @core-features
 * - ci workflow failure detection with keyword filtering
 * - webhook payload validation and error handling
 * - apr service orchestration with isolated error handling
 */

import { Probot } from "probot";
import { SimpleWorkflowEvent } from "../types";
import { MVPAPRService } from "../mvp-apr-service";

/**
 * register workflow_run event handler with probot application
 * filters events early and processes ci failures asynchronously
 * 
 * @param app - probot instance for event registration
 * @performance filters events early to reduce processing overhead
 */
export default (app: Probot) => {
  app.on<"workflow_run">("workflow_run", async (ctx) => {
    ctx.log.info("[workflow-run-handler] workflow_run event received");
    const payload = ctx.payload;

    // validate required fields in webhook payload
    try {
      if (!payload.workflow_run || !payload.repository || !payload.installation) {
        ctx.log.error("[workflow-run-handler] malformed webhook payload: missing required fields");
        return;
      }

      if (!payload.workflow_run.name || !payload.workflow_run.head_sha || !payload.workflow_run.head_branch) {
        ctx.log.error("[workflow-run-handler] malformed workflow_run data: missing required workflow fields");
        return;
      }

      if (!payload.repository.owner?.login || !payload.repository.name) {
        ctx.log.error("[workflow-run-handler] malformed repository data: missing owner or name");
        return;
      }
    } catch (error) {
      ctx.log.error("[workflow-run-handler] error processing webhook payload:", error);
      return;
    }

    // only process completed workflows
    if (payload.action !== "completed") {
      ctx.log.info(`[workflow-run-handler] skipping non-completed action: ${payload.action}`);
      return;
    }

    // only process failed workflows
    if (payload.workflow_run.conclusion !== "failure") {
      ctx.log.info(`[workflow-run-handler] skipping non-failed workflow: ${payload.workflow_run.conclusion}`);
      return;
    }

    // skip workflows created by ci-apr-bot to prevent infinite loops
    const workflowName = payload.workflow_run.name.toLowerCase();
    if (workflowName.includes("ci-apr-bot")) {
      ctx.log.info(`[workflow-run-handler] skipping ci-apr-bot workflow: ${payload.workflow_run.name}`);
      return;
    }

    // filter for ci-related workflows using common keywords
    const ciKeywords = ["ci", "test", "build", "check", "cmake", "gcc", "clang"];
    const isCIWorkflow = ciKeywords.some(keyword => workflowName.includes(keyword));

    if (!isCIWorkflow) {
      ctx.log.info(`[workflow-run-handler] skipping non-ci workflow: ${payload.workflow_run.name}`);
      return;
    }

    // log workflow failure details for debugging
    ctx.log.info(`[workflow-run-handler] detected ci workflow failure: ${payload.workflow_run.name}`);
    ctx.log.info(`[workflow-run-handler] workflow conclusion: ${payload.workflow_run.conclusion}`);
    ctx.log.info(`[workflow-run-handler] head branch: ${payload.workflow_run.head_branch}`);
    ctx.log.info(`[workflow-run-handler] head sha: ${payload.workflow_run.head_sha.substring(0, 8)}...`);

    ctx.log.info(`[workflow-run-handler] processing failed ci workflow: ${payload.workflow_run.name}`);

    const repo = payload.repository;
    const workflowRun = payload.workflow_run;

    // create simplified event object for apr processing
    const event: SimpleWorkflowEvent = {
      installationId: payload.installation!.id,
      repo: { owner: repo.owner.login, name: repo.name },
      workflowName: workflowRun.name,
      headSha: workflowRun.head_sha,
      headBranch: workflowRun.head_branch,
    };

    ctx.log.info(`[workflow-run-handler] triggering apr for ${event.repo.owner}/${event.repo.name}@${event.headSha}`);

    // initialize apr service and process the failed workflow
    const aprService = new MVPAPRService();

    try {
      await aprService.processFailedWorkflow(event);
      ctx.log.info(`[workflow-run-handler] ✓ apr processing completed successfully for ${event.repo.owner}/${event.repo.name}`);
    } catch (error) {
      // handle apr processing errors gracefully to prevent webhook crashes
      ctx.log.error(`[workflow-run-handler] ✗ apr processing failed for ${event.repo.owner}/${event.repo.name}:`, error);

      // log detailed error information for debugging
      if (error instanceof Error) {
        ctx.log.error(`[workflow-run-handler] error details: ${error.message}`);
        if (error.stack) {
          ctx.log.error(`[workflow-run-handler] stack trace: ${error.stack}`);
        }
      }

      // continue processing other events (don't crash the webhook handler)
      ctx.log.info(`[workflow-run-handler] continuing to process other webhook events despite apr failure`);
    }
  });
};