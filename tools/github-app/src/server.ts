/**
 * @file github app server entry point for automated program repair bot
 * @path src/server.ts
 * 
 * minimal probot server that monitors ci workflow failures and triggers apr analysis.
 * handles github webhook events and orchestrates automated program repair workflow.
 * 
 * @core-features
 * - github webhook event processing for workflow failures
 * - environment validation at startup with fail-fast behavior
 * - centralized error handling with actionable error messages
 */

import { Probot } from "probot";
import workflowRunHandler from "./handlers/workflow-run";

/**
 * initialize probot application with apr workflow handler
 * validates environment and registers webhook handlers
 * 
 * @param app - probot instance
 */
export = (app: Probot) => {
  try {
    // log startup information
    app.log.info("starting minimal github apr bot");
    app.log.info("bot will monitor ci workflow failures and trigger apr analysis");

    // validate required environment variables at startup
    const requiredEnvVars = ['APP_ID', 'PRIVATE_KEY', 'WEBHOOK_SECRET'];
    const missingVars = requiredEnvVars.filter(envVar => !process.env[envVar]);

    if (missingVars.length > 0) {
      const errorMessage = `missing required environment variables: ${missingVars.join(', ')}`;
      app.log.error(`✗ startup failed: ${errorMessage}`);
      throw new Error(errorMessage);
    }

    // validate app_id is a valid number
    const appId = process.env.APP_ID;
    if (appId && isNaN(parseInt(appId))) {
      const errorMessage = `invalid app_id: '${appId}' is not a valid number`;
      app.log.error(`✗ startup failed: ${errorMessage}`);
      throw new Error(errorMessage);
    }

    app.log.info("✓ environment variables validated successfully");

    // register workflow handler, core functionality for apr processing
    workflowRunHandler(app);

    app.log.info("✓ apr bot ready - monitoring for ci workflow failures");

  } catch (error) {
    app.log.error("✗ failed to start apr bot:", error);

    // provide actionable error messages for common issues
    if (error instanceof Error) {
      app.log.error(`error details: ${error.message}`);

      if (error.message.includes('environment variables')) {
        app.log.error("please ensure all required environment variables are set in your deployment configuration.");
        app.log.error("required variables: app_id, private_key, webhook_secret");
      }
    }

    // re-throw to prevent the bot from starting in a broken state
    throw error;
  }
}; 