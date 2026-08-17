# Odyssey Task Authoring Rules

Follow these rules when generating, authoring, or validating tasks for Odyssey benchmarks.

## 1. Bundle Directory Structure
All tasks must follow the Terminal-Bench / Harbor bundle layout:
- `task.toml`: Declares `[metadata]`, `[environment]`, `[agent]`, and `[verifier]`. Must be valid TOML.
- `instruction.md`: Substantive, comprehensive problem statement read by the agent. No stubs.
- `environment/Dockerfile`: Builds `/app`. All dependencies must be baked in; no network calls during rollout unless allowlisted.
- `tests/test.sh`: Canonical entrypoint for the sealed verifier.
- `solution/solve.sh`: Canonical entrypoint for the oracle reference solution.

## 2. Configuration & Limits (`task.toml` & Metadata)
- **Slug**: 3–80 lowercase-kebab chars (`^[a-z0-9-]+$`).
- **Collection Family**: One of `Library clone`, `Product clone`, `ML engineering`, `Algorithmic optimization`.
- **Task Family**: One of `feature_development`, `debugging`, `refactoring`, `performance`, `systems_integration`, `other`.
- **Verifier Family**: One of `programmatic`, `optimization`, `ml_artifact`, `custom`.
- **Resources**:
  - `cpuMillis`: 100–64,000 (≤ 8 CPUs)
  - `memoryMb`: 128–262,144 (≤ 65,536 MB)
  - `storageMb`: 128–1,048,576 (≤ 40,960 MB)
  - `gpuCount`: 0–8
  - `agentTimeoutSec`: 7,200–86,400 (≥ 2 hours)
  - `verifierTimeoutSec`: 1–86,400
  - Total trial budget (build + agent + verify + teardown) must be ≤ 50,400s (14h).
  - Values in `task.toml` must be ≤ declared values in draft.
- **Network**:
  - Rollout `[agent].network_mode` must be `none` (default/preferred) or `allowlist` (1–100 hosts + non-empty justification).
  - Unrestricted `open` internet egress is prohibited.

## 3. Verifier & Oracle Standards
- **Oracle Solvability**: `solution/solve.sh` must reliably score ~100% full reward against `tests/test.sh`.
- **NOP Baseline**: Untouched initial state must score 0% (baseline floor).
- **Multi-Channel Grading**: Evaluate multiple independent criteria (functional behavior, invariants, edge cases, performance) rather than single assertions.
- **Split Visibility**: Separate public tests (spec definition for self-checking) from sealed held-out tests to prevent overfitting.
- **Anti-Exploit Hardening**: Prevent hard-coded answers, metric hacking, or leakage of evaluation test cases/datasets.
