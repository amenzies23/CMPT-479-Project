/**
 * @file github app authentication service
 * @path src/services/auth-service.ts
 * 
 * handles github app authentication and installation token management.
 * provides secure token acquisition with comprehensive error handling.
 * 
 * @core-features
 * - github app authentication with private key validation
 * - installation token acquisition with timeout protection
 * - comprehensive error handling with actionable messages
 */

import { createAppAuth } from '@octokit/auth-app';

export class AuthService {
  /**
   * get github app installation token for api access
   * validates credentials and handles common authentication errors
   * 
   * @param installationId - github app installation id
   * @returns promise resolving to access token
   */
  async getInstallationToken(installationId: number): Promise<string> {
    console.log(`getting installation token for installation: ${installationId}`);
    
    try {
      // validate installation id parameter
      if (!installationId || installationId <= 0) {
        throw new Error(`invalid installation id: ${installationId}. must be a positive number.`);
      }

      // get required environment variables
      const appId = process.env.APP_ID!;
      const privateKey = process.env.PRIVATE_KEY!;
      
      // validate private key format to catch common configuration errors
      if (!privateKey.includes('BEGIN') || !privateKey.includes('END')) {
        throw new Error('private_key appears to be malformed. ensure it includes begin and end markers.');
      }
      
      console.log(`creating github app authentication for app id: ${appId}`);
      
      // create github app authentication instance
      const auth = createAppAuth({
        appId: parseInt(appId),
        privateKey: privateKey.replace(/\\n/g, '\n'), // handle escaped newlines from env vars
      });
      
      console.log(`requesting installation token for installation: ${installationId}`);
      
      // get installation access token with timeout protection
      const installationAuth = await Promise.race([
        auth({
          type: 'installation',
          installationId: installationId,
        }),
        new Promise((_, reject) => 
          setTimeout(() => reject(new Error('github authentication timeout after 30 seconds')), 30000)
        )
      ]) as any;
      
      if (!installationAuth.token) {
        throw new Error('github authentication succeeded but no token was returned');
      }
      
      console.log('✓ installation token obtained successfully');
      return installationAuth.token;
      
    } catch (error) {
      console.error('✗ failed to get installation token:', error);
      
      // provide specific error messages for common authentication issues
      if (error instanceof Error) {
        if (error.message.includes('timeout')) {
          throw new Error('github authentication timed out. check network connectivity and github api status.');
        } else if (error.message.includes('401')) {
          throw new Error('github authentication failed: invalid app_id or private_key. verify your github app credentials.');
        } else if (error.message.includes('404')) {
          throw new Error(`github app installation not found: installation id ${installationId} may be invalid or the app may not be installed.`);
        } else if (error.message.includes('403')) {
          throw new Error('github authentication failed: insufficient permissions or rate limit exceeded.');
        }
      }
      
      throw new Error(`failed to authenticate with github app: ${error instanceof Error ? error.message : 'unknown authentication error'}`);
    }
  }
}