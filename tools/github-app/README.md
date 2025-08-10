# GITHUB APR Bot

automated program repair bot for github ci failures

## overview

the github apr bot monitors ci workflow failures and automatically generates patches using automated program repair (our project! @https://github.com/amenzies23/CMPT-479-Project). when a ci workflow fails, the bot clones the repository, runs apr analysis, and creates a pull request with potential fixes.

## features

- **ci failure detection**, monitors github workflows for test failures
- **automated analysis** → runs apr binary on failed repositories
- →creates code patches for identified issues
- submits patches as github pull requests

## architecture

```
github webhook → workflow handler → our apr engine → pull request
                      ↓
                 event filtering
                      ↓
                 payload validation
                      ↓
                 apr orchestration
```

