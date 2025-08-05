/**
 * @file pull request creation service
 * @path src/services/pull-request-service.ts
 * 
 * handles github pull request creation and branch management for apr patches.
 * creates branches, applies patches as commits, and generates comprehensive pr descriptions.
 * 
 * @core-features
 * - branch creation with fallback naming for conflicts
 * - patch application as individual commits with detailed messages
 * - comprehensive pr descriptions with confidence analysis
 * - error handling with github api specific error messages
 */

import { Octokit } from '@octokit/rest';
import { SimpleWorkflowEvent, APRResult, Patch } from '../types';

export class PullRequestService {
  private octokit: Octokit;

  constructor(token: string) {
    this.octokit = new Octokit({ auth: token });
  }

  /**
   * create pull request with generated patches
   * orchestrates branch creation, patch application, and pr creation
   * 
   * @param event - workflow event containing repository information
   * @param aprResult - apr analysis results with patches
   * @returns promise that resolves to pr url
   */
  async createPullRequest(event: SimpleWorkflowEvent, aprResult: APRResult): Promise<string> {
    console.log(`creating pull request with ${aprResult.patches.length} patches`);

    try {
      // step 1: create new branch with descriptive name
      const branchName = await this.createBranch(event, aprResult);
      console.log(`✓ created branch: ${branchName}`);

      // step 2: apply patches as individual commits
      await this.applyPatchesAsCommits(event, aprResult, branchName);
      console.log(`✓ applied ${aprResult.patches.length} patches as commits`);

      // step 3: create pull request with analysis summary
      const prUrl = await this.createPullRequestWithSummary(event, aprResult, branchName);
      console.log(`✓ created pull request: ${prUrl}`);

      return prUrl;

    } catch (error) {
      console.error('✗ failed to create pull request:', error);

      let errorMessage = 'unknown pull request creation error';

      if (error instanceof Error) {
        if (error.message.includes('422')) {
          errorMessage = 'failed to create pull request: branch may already exist, validation failed, or repository settings prevent pr creation';
        } else if (error.message.includes('403')) {
          errorMessage = 'failed to create pull request: insufficient permissions, rate limit exceeded, or repository is archived/disabled';
        } else if (error.message.includes('404')) {
          errorMessage = 'failed to create pull request: repository not found, not accessible, or github app not properly installed';
        } else if (error.message.includes('401')) {
          errorMessage = 'failed to create pull request: authentication failed - token may have expired';
        } else if (error.message.includes('409')) {
          errorMessage = 'failed to create pull request: conflict with existing branch or pr';
        } else if (error.message.includes('timeout')) {
          errorMessage = 'failed to create pull request: github api request timed out';
        } else {
          errorMessage = `failed to create pull request: ${error.message}`;
        }
      }

      console.error(`✗ pull request creation failed: ${errorMessage}`);
      throw new Error(errorMessage);
    }
  }

  /**
   * create a new branch for the apr patches
   * generates unique branch name and creates from head commit
   * branch name: apr/fix-ci-${short_sha}
   * 
   * @param event - workflow event with repo info
   * @param aprResult - apr results
   * @returns promise resolving to created branch name
   */
  private async createBranch(event: SimpleWorkflowEvent, aprResult: APRResult): Promise<string> {
    console.log('creating new branch for apr patches...');

    // generate clean branch name with commit hash
    const shortSha = event.headSha.substring(0, 8);
    const branchName = `apr/fix-ci-${shortSha}`;

    try {
      console.log(`branch name: ${branchName}`);

      // get the current commit sha from the head branch
      const { data: refData } = await this.octokit.rest.git.getRef({
        owner: event.repo.owner,
        repo: event.repo.name,
        ref: `heads/${event.headBranch}`
      });

      const baseSha = refData.object.sha;
      console.log(`base sha: ${baseSha}`);

      // create new branch from the head commit
      await this.octokit.rest.git.createRef({
        owner: event.repo.owner,
        repo: event.repo.name,
        ref: `refs/heads/${branchName}`,
        sha: baseSha
      });

      console.log(`✓ created branch ${branchName} from ${baseSha}`);
      return branchName;

    } catch (error) {
      console.error('✗ failed to create branch:', error);

      if (error instanceof Error && error.message.includes('422')) {
        // branch might already exist, try with a different name
        const fallbackBranchName = `apr/fix-ci-${shortSha}-${Math.random().toString(36).substring(2, 5)}`;
        console.log(`retrying with fallback branch name: ${fallbackBranchName}`);

        try {
          const { data: refData } = await this.octokit.rest.git.getRef({
            owner: event.repo.owner,
            repo: event.repo.name,
            ref: `heads/${event.headBranch}`
          });

          await this.octokit.rest.git.createRef({
            owner: event.repo.owner,
            repo: event.repo.name,
            ref: `refs/heads/${fallbackBranchName}`,
            sha: refData.object.sha
          });

          console.log(`✓ created fallback branch: ${fallbackBranchName}`);
          return fallbackBranchName;

        } catch (fallbackError) {
          throw new Error(`✗ failed to create branch even with fallback name: ${fallbackError instanceof Error ? fallbackError.message : 'unknown error'}`);
        }
      }

      throw new Error(`✗ failed to create branch: ${error instanceof Error ? error.message : 'unknown error'}`);
    }
  }

  /**
   * apply patches as individual commits to the branch
   * creates one commit per patch with detailed commit messages
   * 
   * @param event - workflow event with repository information
   * @param aprResult - apr results containing patches to apply
   * @param branchName - target branch for commits
   */
  private async applyPatchesAsCommits(
    event: SimpleWorkflowEvent,
    aprResult: APRResult,
    branchName: string
  ): Promise<void> {
    console.log(`applying ${aprResult.patches.length} patches as commits...`);

    for (let i = 0; i < aprResult.patches.length; i++) {
      const patch = aprResult.patches[i];
      if (!patch) {
        console.error(`✗ patch ${i + 1} is undefined, skipping`);
        continue;
      }

      console.log(`applying patch ${i + 1}/${aprResult.patches.length}: ${patch.file}:${patch.line}`);

      try {
        // get current file content
        let fileContent: string;
        let fileSha: string | undefined;

        try {
          const { data: fileData } = await this.octokit.rest.repos.getContent({
            owner: event.repo.owner,
            repo: event.repo.name,
            path: patch.file,
            ref: branchName
          });

          if ('content' in fileData && fileData.content) {
            fileContent = Buffer.from(fileData.content, 'base64').toString('utf-8');
            fileSha = fileData.sha;
          } else {
            throw new Error('file content not found or is a directory');
          }
        } catch (fileError) {
          console.warn(`could not read file ${patch.file}, creating new file`);
          fileContent = '';
          fileSha = undefined;
        }

        // apply patch to file content
        const modifiedContent = this.applyPatchToContent(fileContent, patch);

        // create commit with the modified file
        const commitMessage = this.generateCommitMessage(patch, i + 1);

        const updateParams: any = {
          owner: event.repo.owner,
          repo: event.repo.name,
          path: patch.file,
          message: commitMessage,
          content: Buffer.from(modifiedContent).toString('base64'),
          branch: branchName
        };

        if (fileSha) {
          updateParams.sha = fileSha;
        }

        await this.octokit.rest.repos.createOrUpdateFileContents(updateParams);

        console.log(`✓ applied patch ${i + 1}: ${patch.file}:${patch.line}`);

      } catch (error) {
        console.error(`✗ failed to apply patch ${i + 1} (${patch.file}:${patch.line}):`, error);
        continue;
      }
    }

    console.log(`✓ finished applying patches to branch ${branchName}`);
  }

  /**
   * apply a patch to file content
   * handles line replacement with bounds checking and content validation
   * 
   * @param fileContent - original file content as string
   * @param patch - patch containing line number and replacement content
   * @returns modified file content with patch applied
   */
  private applyPatchToContent(fileContent: string, patch: Patch): string {
    const lines = fileContent.split('\n');

    // handle case where file is empty or patch line is beyond file length
    if (patch.line <= 0 || patch.line > lines.length) {
      console.warn(`patch line ${patch.line} is out of bounds for file with ${lines.length} lines`);

      if (patch.line > lines.length) {
        // add empty lines if needed
        while (lines.length < patch.line - 1) {
          lines.push('');
        }
        lines.push(patch.fixed);
      } else {
        // if line is <= 0, prepend the fixed content
        lines.unshift(patch.fixed);
      }
    } else {
      // replace the line at the specified position (1-indexed)
      const targetLineIndex = patch.line - 1;
      const currentLine = lines[targetLineIndex];

      // check if the original content matches (for safety)
      if (patch.original && patch.original.trim() !== '' &&
        currentLine && !currentLine.includes(patch.original.trim()) &&
        !patch.original.includes('//')) {
        console.warn(`original content mismatch at line ${patch.line}. expected: "${patch.original.trim()}", found: "${currentLine.trim()}"`);
        lines[targetLineIndex] = `${patch.fixed} // apr: original content may have changed`;
      } else {
        lines[targetLineIndex] = patch.fixed;
      }
    }

    return lines.join('\n');
  }

  /**
   * generate commit message for a patch
   * creates detailed commit message with confidence and context information
   * 
   * @param patch - patch being applied
   * @param patchNumber - sequential number of the patch
   * @returns formatted commit message following conventional commits
   */
  private generateCommitMessage(patch: Patch, patchNumber: number): string {
    const confidence = Math.round(patch.confidence * 100);
    const fileName = patch.file.split('/').pop() || patch.file; // get just the filename

    return `fix(ci-apr-bot): resolve issue in ${fileName}:${patch.line}

Applied automated program repair patch with ${confidence}% confidence.
This patch addresses a potential issue identified by static analysis.

- file: ${patch.file}
- line: ${patch.line}
- confidence: ${confidence}%
- patch: ${patchNumber}`;
  }

  /**
   * create pull request with analysis summary
   * generates comprehensive pr with confidence analysis and patch details
   * 
   * @param event - workflow event with repository information
   * @param aprResult - apr results with patches and confidence scores
   * @param branchName - source branch for the pull request
   * @returns promise resolving to created pr url
   */
  private async createPullRequestWithSummary(
    event: SimpleWorkflowEvent,
    aprResult: APRResult,
    branchName: string
  ): Promise<string> {
    console.log('creating pull request with analysis summary...');

    try {
      // generate pr title and body
      const title = this.generatePRTitle(aprResult, event);
      const body = this.generatePRBody(aprResult, event);

      console.log(`pr title: ${title}`);
      console.log(`pr body length: ${body.length} characters`);

      // create the pull request
      const { data: pr } = await this.octokit.rest.pulls.create({
        owner: event.repo.owner,
        repo: event.repo.name,
        title: title,
        body: body,
        head: branchName,
        base: event.headBranch,
        draft: false
      });

      console.log(`✓ created pull request #${pr.number}: ${pr.html_url}`);
      return pr.html_url;

    } catch (error) {
      console.error('✗ failed to create pull request:', error);
      throw new Error(`✗ failed to create pull request: ${error instanceof Error ? error.message : 'unknown error'}`);
    }
  }

  /**
   * generate pull request title
   * creates concise title with patch count and average confidence
   * 
   * @param aprResult - apr results with patches
   * @param event - workflow event (unused but kept for consistency)
   * @returns formatted pr title
   */
  private generatePRTitle(aprResult: APRResult, event: SimpleWorkflowEvent): string {
    const patchCount = aprResult.patches.length;
    const avgConfidence = aprResult.patches.length > 0
      ? Math.round((aprResult.patches.reduce((sum, p) => sum + p.confidence, 0) / aprResult.patches.length) * 100)
      : 0;

    return `ci-apr-bot: fix CI failure (${patchCount} ${patchCount === 1 ? 'patch' : 'patches'}, ${avgConfidence}% confidence)`;
  }

  /**
   * generate pull request body with analysis summary
   * creates comprehensive markdown description with confidence breakdown
   * 
   * @param aprResult - apr results with patches and confidence scores
   * @param event - workflow event with context information
   * @returns formatted markdown pr body
   */
  private generatePRBody(aprResult: APRResult, event: SimpleWorkflowEvent): string {
    const patchCount = aprResult.patches.length;
    const avgConfidence = aprResult.patches.length > 0
      ? Math.round((aprResult.patches.reduce((sum, p) => sum + p.confidence, 0) / aprResult.patches.length) * 100)
      : 0;

    const highConfidencePatches = aprResult.patches.filter(p => p.confidence >= 0.8).length;
    const mediumConfidencePatches = aprResult.patches.filter(p => p.confidence >= 0.6 && p.confidence < 0.8).length;
    const lowConfidencePatches = aprResult.patches.filter(p => p.confidence < 0.6).length;

    let body = `## APR Bot Analysis

This pull request contains automated fixes generated in response to the failure in CI workflow.

### ☞ Analysis Summary

- **Patches Generated**: ${patchCount}
- **Average Confidence**: ${avgConfidence}%
- **Trigger**: CI failure on commit \`${event.headSha.substring(0, 8)}\` ([View full logs](${event.workflowUrl || '#'}))
- **Base Branch**: \`${event.headBranch}\`

### ☞ Test Results

// TODO: add test results here

| Scenario         | Before                | After                 |
|------------------|-----------------------|-----------------------|
| \`CI Build\`     | Failed                | Expected to pass      |
| Coverage Change  | N/A                   | To be verified        |

### ☞ Confidence Breakdown

// TODO: update this to reflect our actual confidence breakdown (it's mocked for now)

| Confidence Level     | Count | Description                  |
|----------------------|-------|------------------------------|
| High (≥80%)          | ${highConfidencePatches}     | Strongly validated patches   |
| Medium (60–79%)      | ${mediumConfidencePatches}     | Moderate validation          |
| Low (<60%)           | ${lowConfidencePatches}     | Review carefully             |

### ☞ Applied Patches

`;

    // add details for each patch
    aprResult.patches.forEach((patch, index) => {
      const confidence = Math.round(patch.confidence * 100);
      const confidenceEmoji = confidence >= 80 ? '🟢' : confidence >= 60 ? '🟡' : '🔴';
      const patchType = this.determinePatchType(patch);
      const riskLevel = confidence >= 80 ? 'low' : confidence >= 60 ? 'medium' : 'high';

      body += `#### ${confidenceEmoji} patch ${index + 1}: \`${patch.file}\`

- **type**: ${patchType}
- **scope**: core module
- **risk**: ${riskLevel} (${this.getRiskDescription(patch, confidence)})
- **location**: line ${patch.line}
- **confidence**: ${confidence}%

\`\`\`diff
- ${patch.original || '// original code at line ' + patch.line}
+ ${patch.fixed} // fixed: ${this.getFixDescription(patch)}
\`\`\`

`;
    });

    body += `### ⚠️ Important Notes

- **review required!** double-check logic and edge cases.
- **do manual testing!**: run your full test suite locally.
- **see confidence scores**, higher-confidence patches are more likely correct.
- **check APR docs**, see [how apr works](${this.getAPRDocsUrl()}) for details on patch ranking and confidence.

### ☞ Next Steps

1. Review changes in each commit
2. Verify tests pass locally
3. Merge if satisfied, or close and rerun APR if not suitable

This PR was automatically generated by ci-apr-bot in response to a CI failure.`;

    return body;
  }

  /**
   * TODO: determine patch type based on content
   * @param patch - patch to analyze 
   * @returns patch type classification
   */
  private determinePatchType(patch: Patch): string {
    return 'fix';
  }

  /**
   * TODO: get risk description based on confidence score
   * simplified risk assessment based only on confidence level
   * 
   * @param patch - patch to assess
   * @param confidence - confidence score as percentage
   * @returns risk description string
   */
  private getRiskDescription(patch: Patch, confidence: number): string {
    if (confidence >= 80) {
      return 'low risk, well-validated change';
    }
    if (confidence >= 60) {
      return 'medium risk';
    }
    return 'high risk, review required';
  }

  /**
   * TODO: get fix description for patch
   * simplified to return generic description
   * 
   * @param patch - patch to describe)
   * @returns fix description string
   */
  private getFixDescription(patch: Patch): string {
    return 'automated fix';
  }

  /**
   * placeholder for future documentation link
   * 
   * @returns documentation url (currently placeholder)
   */
  private getAPRDocsUrl(): string {
    return '#'; // TODO: replace with actual documentation url when we will finish impl
  }
}