/**
 * @file cleanup management utilities
 * @path src/utils/cleanup-manager.ts
 * 
 * manages resource cleanup and process termination handlers for graceful shutdown.
 * provides centralized cleanup coordination with error handling and signal management.
 * 
 * @core-features
 * - callback registration for cleanup tasks
 * - process signal handling with graceful shutdown
 * - error isolation during cleanup execution
 * - test environment compatibility with signal handling
 */

export class CleanupManager {
  private cleanupCallbacks: (() => Promise<void>)[] = [];
  private cleanupRegistered: boolean = false;

  /**
   * register a cleanup callback for execution during shutdown
   * callbacks are executed in registration order during cleanup
   * 
   * @param callback - async function to execute during cleanup
   */
  registerCleanup(callback: () => Promise<void>): void {
    this.cleanupCallbacks.push(callback);
  }

  /**
   * register cleanup handlers for process termination signals
   * handles sigint, sigterm, sigquit, uncaught exceptions, and unhandled rejections
   * 
   * @performance skips registration in test environments to avoid listener warnings
   */
  registerCleanupHandlers(): void {
    if (this.cleanupRegistered) {
      return; // avoid registering multiple times
    }
    
    // skip cleanup handler registration during testing to avoid warning
    if (process.env.NODE_ENV === 'test' || process.env.JEST_WORKER_ID) {
      this.cleanupRegistered = true;
      return;
    }
    
    const signals: NodeJS.Signals[] = ['SIGINT', 'SIGTERM', 'SIGQUIT'];
    
    // register handlers for standard termination signals
    signals.forEach(signal => {
      process.on(signal, async () => {
        console.log(`\n🛑 received ${signal}, performing cleanup before exit...`);
        try {
          await this.executeCleanup();
          console.log('✓ cleanup completed, exiting gracefully');
          process.exit(0);
        } catch (cleanupError) {
          console.error('✗ cleanup failed during process termination:', cleanupError);
          process.exit(1);
        }
      });
    });
    
    // handle uncaught exceptions with emergency cleanup
    process.on('uncaughtException', async (error) => {
      console.error('⚠️ uncaught exception:', error);
      try {
        await this.executeCleanup();
        console.log('✓ emergency cleanup completed');
      } catch (cleanupError) {
        console.error('✗ emergency cleanup failed:', cleanupError);
      }
      process.exit(1);
    });
    
    // handle unhandled promise rejections with emergency cleanup
    process.on('unhandledRejection', async (reason, promise) => {
      console.error('⚠️ unhandled promise rejection at:', promise, 'reason:', reason);
      try {
        await this.executeCleanup();
        console.log('✓ emergency cleanup completed');
      } catch (cleanupError) {
        console.error('✗ emergency cleanup failed:', cleanupError);
      }
      process.exit(1);
    });
    
    this.cleanupRegistered = true;
    console.log('✓ cleanup handlers registered for process termination');
  }

  /**
   * execute all registered cleanup callbacks
   * runs callbacks sequentially with error isolation and comprehensive logging
   * 
   * @performance executes callbacks in registration order with error isolation
   */
  async executeCleanup(): Promise<void> {
    console.log('starting comprehensive resource cleanup...');
    
    const cleanupErrors: string[] = [];
    
    // execute each cleanup callback with error isolation
    for (const callback of this.cleanupCallbacks) {
      try {
        await callback();
      } catch (error) {
        const errorMsg = `cleanup callback failed: ${error instanceof Error ? error.message : 'unknown error'}`;
        cleanupErrors.push(errorMsg);
        console.error(`✗ ${errorMsg}`);
      }
    }
    
    // provide summary of cleanup results
    if (cleanupErrors.length === 0) {
      console.log('✓ all resources cleaned up successfully');
    } else {
      console.warn(`⚠️ cleanup completed with ${cleanupErrors.length} warnings:`);
      cleanupErrors.forEach((error, index) => {
        console.warn(`  ${index + 1}. ${error}`);
      });
    }
  }
}