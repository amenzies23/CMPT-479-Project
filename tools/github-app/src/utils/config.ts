/**
 * @file configuration management utilities
 * @path src/utils/config.ts
 *
 * centralized configuration management with validation and type safety.
 * handles environment variables and provides structured configuration access.
 *
 * - fail-fast validation prevents runtime errors
 * - typed configuration prevents misuse
 * - clear error messages for missing configuration
 */

import { ErrorCategory, createError } from './error-handler';

/**
 * application configuration interface
 */
export interface AppConfig {
  github: {
    appId: number;
    privateKey: string;
    webhookSecret: string;
  };
  apr: {
    binaryPaths: string[];
    downloadUrl: string;
    timeout: number;
    maxPatches: number;
  };
  server: {
    port: number;
    webhookPath: string;
  };
}

/**
 * validate and load application configuration
 * 
 * @returns validated configuration object
 * @throws APRError if configuration is invalid
 * 
 * @performance loads configuration once at start
 */
export function loadConfig(): AppConfig {
  const requiredEnvVars = ['APP_ID', 'PRIVATE_KEY', 'WEBHOOK_SECRET'];
  const missingVars: string[] = [];

  // check for required evn variables
  for (const envVar of requiredEnvVars) {
    if (!process.env[envVar]) {
      missingVars.push(envVar);
    }
  }

  if (missingVars.length > 0) {
    throw createError(
      ErrorCategory.CONFIGURATION,
      `missing required environment variables: ${missingVars.join(', ')}`,
      undefined,
      { missingVars }
    );
  }

  // validate app_id is a valid number
  const appId = process.env.APP_ID!;
  const parsedAppId = parseInt(appId);
  if (isNaN(parsedAppId)) {
    throw createError(
      ErrorCategory.CONFIGURATION,
      `invalid APP_ID: '${appId}' is not a valid number`,
      undefined,
      { appId }
    );
  }

  // validate private key format
  const privateKey = process.env.PRIVATE_KEY!;
  if (!privateKey.includes('BEGIN') || !privateKey.includes('END')) {
    throw createError(
      ErrorCategory.CONFIGURATION,
      'PRIVATE_KEY appears to be malformed - ensure it includes BEGIN and END markers',
      undefined,
      { keyLength: privateKey.length }
    );
  }

  return {
    github: {
      appId: parsedAppId,
      privateKey: privateKey.replace(/\\n/g, '\n'), // handle escaped newlines
      webhookSecret: process.env.WEBHOOK_SECRET!
    },
    apr: {
      binaryPaths: [
        '/app/bin/apr_system',           // production (heroku)
        './bin/apr_system',              // local deployment
        process.cwd() + '/bin/apr_system' // current working directory
      ],
      downloadUrl: 'https://github.com/arusinova/apr-project/releases/download/v1.0.4',
      timeout: parseInt(process.env.APR_TIMEOUT || '300000'), // 5 minutes default
      maxPatches: parseInt(process.env.APR_MAX_PATCHES || '10')
    },
    server: {
      port: parseInt(process.env.PORT || '3000'),
      webhookPath: process.env.WEBHOOK_PATH || '/webhook'
    }
  };
}

/**
 * get configuration instance
 * 
 * @returns application configuration
 * 
 * @performance configuration loaded once and cached
 */
let configInstance: AppConfig | null = null;

export function getConfig(): AppConfig {
  if (!configInstance) {
    configInstance = loadConfig();
  }
  return configInstance;
}