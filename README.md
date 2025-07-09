# CMPT 479 APR System

## Architecture

```
Input → SBFL → Parser → Mutator → Prioritizer → Validator → PRBot → Output
         ↓       ↓        ↓          ↓           ↓         ↓
    Suspicious  AST     Patch      Ranked     Validated  GitHub
    Locations  Nodes  Candidates  Patches     Patches     PR
```

## Building the Project

### 1. Clone and Navigate
```bash
git clone <repository-url>
cd CMPT-479-Project
```

### 2. Create Out-of-Source Build Directory

### 3. Build the Project

### 4. Build Directory Structure
After building, your `build/` directory will be organized as:
```
build/
├── bin/
│   ├── apr_system        # Main executable
│   └── apr_tests         # Test executable
├── lib/
│   └── libapr_system_lib.a # Static library
├── logs/                 # Log files (auto-created)
├── results/              # Output results (auto-created)
└── CMakeFiles/           # CMake build artifacts
```

## Usage

### Basic Usage
```bash
# from build directory
./bin/apr_system --repo-url "https://github.com/user/repo" (currently with mock data)
```

### Complete Example
```bash
./bin/apr_system \
  --repo-url "https://github.com/user/repo" \
  --output-dir "./results" \
  --verbose
```

## Running Tests

### Run All Tests
```bash
# from build directory
./bin/apr_tests
```

### Run Specific Test Suites
```bash
# run with filters
./bin/apr_tests --gtest_filter="*SBFL*"
./bin/apr_tests --gtest_filter="*Validator*"
```

## Output Files

### Pipeline Results
```bash
# main results file (JSON format)
./results/pipeline_results.json
```

**Example Output Structure:**
```json
{
  "pipeline_summary": {
    "suspicious_locations_count": 2,
    "patch_candidates_count": 2,
    "validation_results_count": 2,
    "pr_created": true
  },
  "repository_metadata": {
    "repository_url": "https://github.com/user/repo",
    "branch": "main",
    "commit_hash": "abc123"
  },
  "suspicious_locations": [...],
  "validation_results": [...],
  "pr_result": {
    "success": true,
    "pr_url": "https://github.com/user/repo/pull/123"
  }
}
```

### Execution Logs
```bash
# detailed logs with timestamps
./logs/apr_system.log
```

## 📁 Project Structure

```
CMPT-479-Project/
├── src/                    # source code
│   ├── main.cpp            # entry point
│   ├── cli/                # command line interface
│   ├── core/               # core utilities (logger, types)
│   ├── sbfl/               # spectrum-based fault localization (SBFL)
│   ├── parser/             # AST parsing and analysis
│   ├── mutator/            # patch generation
│   ├── prioritizer/        # patch ranking
│   ├── validator/          # patch validation
│   ├── prbot/              # PR creation
│   └── orchestrator/       # pipeline coordination
├── tests/                  
│   ├── unit/               # unit tests for each module
│   ├── integration/        # integration tests
│   └── fixtures/           # test data and fixtures
├── docs/                   # documentation
└── tools/                  # here will be github app impl
```

## Pipeline Workflow

### 1. **SBFL (spectrum-based fault localization)**
- analyzes test results and coverage data
- identifies suspicious code locations
- **output:** ranked list of potential fault locations

### 2. **parser**
- parses source files into AST
- extracts relevant code structures
- **output:** AST nodes for suspicious locations

### 3. **mutator**
- generates patch candidates using mutation operators
- creates various fix alternatives
- **output:** set of potential patches

### 4. **prioritizer**
- ranks patches based on confidence metrics
- considers fault localization scores
- **output:** prioritized patch list

### 5. **validator**
- compiles and tests each patch
- validates functionality and correctness
- **output:** validation results with pass/fail status

### 6. **PR bot**
- creates pull requests for valid patches
- formats patch descriptions and metadata
- **output:** github pull request PR
