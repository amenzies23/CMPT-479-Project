/**
 * @file centralized error handling utilities
 * @path src/utils/error-handler.ts
 *
 * provides structured error handling
 * standardizes error reporting across the app
 */

/**
 * error categories for classification and handling
 */
export enum ErrorCategory {
  AUTHENTICATION = 'authentication',
  NETWORK = 'network',
  FILESYSTEM = 'filesystem',
  VALIDATION = 'validation',
  APR_EXECUTION = 'apr_execution',
  GITHUB_API = 'github_api',
  CONFIGURATION = 'configuration'
}

/**
 * structured error with category and context
 */
export interface APRError {
  category: ErrorCategory;
  message: string;
  originalError?: Error;
  context?: Record<string, any>;
  actionable?: string;
}

/**
 * create structured error with context
 * 
 * @param category error classification
 * @param message human-readable error description
 * @param originalError source error if available
 * @param context additional error context
 * @returns structured error object
 */
export function createError(
  category: ErrorCategory,
  message: string,
  originalError?: Error,
  context?: Record<string, any>
): APRError {
  const error: APRError = {
    category,
    message
  };
  
  if (originalError) {
    error.originalError = originalError;
  }
  
  if (context) {
    error.context = context;
  }

  // add actionable guidance based on error category
  switch (category) {
    case ErrorCategory.AUTHENTICATION:
      error.actionable = 'verify APP_ID and PRIVATE_KEY configuration';
      break;
    case ErrorCategory.NETWORK:
      error.actionable = 'check network connectivity and github api status';
      break;
    case ErrorCategory.FILESYSTEM:
      error.actionable = 'verify file permissions and disk space';
      break;
    case ErrorCategory.VALIDATION:
      error.actionable = 'check input data format and required fields';
      break;
    case ErrorCategory.APR_EXECUTION:
      error.actionable = 'verify apr binary availability and repository structure';
      break;
    case ErrorCategory.GITHUB_API:
      error.actionable = 'check github api rate limits and permissions';
      break;
    case ErrorCategory.CONFIGURATION:
      error.actionable = 'verify environment variables and configuration';
      break;
  }

  return error;
}

/**
 * log structured error with appropriate level
 * 
 * @param error structured error to log
 * @param logger logging function (console.error by default)
 */
export function logError(error: APRError, logger = console.error): void {
  const logData = {
    category: error.category,
    message: error.message,
    context: error.context,
    actionable: error.actionable,
    stack: error.originalError?.stack,
    timestamp: new Date().toISOString()
  };

  logger('✗ APR error:', JSON.stringify(logData, null, 2));
}

/**
 * wrap function execution with error handling
 * 
 * @param fn function to execute
 * @param category error category for failures
 * @param context additional context for errors
 * @returns wrapped function result or throws APRError
 */
export async function withErrorHandling<T>(
  fn: () => Promise<T>,
  category: ErrorCategory,
  context?: Record<string, any>
): Promise<T> {
  try {
    return await fn();
  } catch (error) {
    const aprError = createError(
      category,
      error instanceof Error ? error.message : 'unknown error',
      error instanceof Error ? error : undefined,
      context
    );
    
    logError(aprError);
    throw aprError;
  }
}