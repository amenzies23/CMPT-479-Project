/**
 * @file execution utilities
 * @path src/utils/exec-utils.ts
 * 
 * utility functions for executing shell commands
 */

import { exec } from 'child_process';
import { promisify } from 'util';

export const execAsync = promisify(exec);