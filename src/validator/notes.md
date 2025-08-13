# validator module

>>> For the MVP implementation, we deliberately adopted minimal, pragmatic choices to prioritize end-to-end functionality over full generality. Directory changes use a simple `chdir`-based RAII guard, assuming single-threaded execution (!), and XML parsing is implemented with a lightweight string scan sufficient for GoogleTest’s stable output format. Timeouts are enforced using POSIX `fork`/`exec` with process group termination for reliable cleanup. These trade-offs enable rapid validation flow integration while deferring complex concurrency, fully hardened parsing, and advanced isolation (such as containerized builds) to future work.

the validator implements capgen's two-phase patch validation methodology. it validates patch candidates against a time budget (currently, 70 minutes) with early termination on success.

## validation flow

**PHASE A (fast filter)**: applies patch, builds project, runs only originally failing tests. if tests still fail, patch is rejected immediately.

**PHASE B (regression check)**: if phase a passes, re-applies patch and runs full regression test suite. if all tests pass, patch is marked as plausible.

## architecture

```
patch candidates → validator → validation results
                      ↓
            ┌─────────────────────┐
            │   two-phase flow    │
            └─────────────────────┘
                      ↓
    ┌─────────────────────────────────────┐
    │          PHASE A (filter)           │
    │  apply patch → build → run failing  │
    │           tests only                │
    └─────────────────────────────────────┘
                      ↓
              ┌─────────────┐
              │ tests pass? │
              └─────────────┘
                 ↓        ↓
               yes       no → reject patch
                 ↓
    ┌─────────────────────────────────────┐
    │      PHASE B (regression)           │
    │ re-apply patch → build → run full   │
    │           test suite                │
    └─────────────────────────────────────┘
                      ↓
              ┌─────────────┐
              │ tests pass? │
              └─────────────┘
                 ↓        ↓
            plausible   regression detected (reject)
```

## key features

- we have a **time budget enforcement**, 70-minute maximum with early exit on success
- automatic directory restoration and file cleanup  
- also, have a detailed phase timing and error reporting
- atomic patch application with rollback capability
- **gtest**: uses `--gtest_filter` for targeted test execution and `--gtest_output=xml` for structured results
- **artifacts**: gtest xml files are written to `./artifacts/gtest/<phase>-<patch_id>.xml` with unique paths per patch/phase
- **timeouts**: hard timeouts are enforced using POSIX fork/exec with SIGTERM/SIGKILL process group termination
- uses git-based restoration (`git restore --source=HEAD`), falls back to manual method
- build logs and gtest xml are attached to validation results for reproducibility
- design assumes single-threaded validator execution to avoid chdir-based working directory race conditions
- runs directly in the repo directory with rollback. no container-level isolation yet (!). build artifacts may persist between runs (accepted for MVP scope)

the validator now creates the following artifact structure:
```text
./artifacts/gtest/
├── phase-a-patch_001.xml
├── phase-b-patch_001.xml
├── phase-a-patch_002.xml
└── phase-b-patch_002.xml
```
