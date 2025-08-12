## Overview

This repository now supports automated program repair (APR) for multiple buggy programs using a new local testing suite.

## Key Changes

### 1. **Testing Suite**
- Added a bash script to build with coverage for each buggy program in `./buggy-programs`.
- The script removes the build directory rebuilds with coverage.

### 2. **CLI Argument Support**
- The APR system binary (`./bin/apr_system`) now accepts the argument:
  ```
  --buggy-program /workspace/buggy-programs/01-buggy-calculator
  ```
- This allows you to specify which buggy program to analyze.
- I have not done any error handling with the passed in argument. Ensure that it follows the example above.

## Usage Example

1. **Run the testing suite script in buggy-programs directory:**
   ```sh
   ./build-all.sh
   ```

2. **Run APR system in build directory:**
   ```sh
   ./bin/apr_system --buggy-program /workspace/buggy-programs/01-buggy-calculator
   ```

## Notes

- This setup is **only tested in a Docker container** using `/workspace` as the project root.
- The CLI and scripts may not work as expected outside Docker or with a different directory structure.