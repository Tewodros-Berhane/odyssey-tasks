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

---

## 6. Common Automated Validation Errors & Prevention Guardrails

To prevent automated validation rejections during the intake and oracle/nop execution stages, every task bundle must strictly adhere to these 5 rules:

### 1. Dockerfile Build Context & Package Availability
- **Build Context Rule**: The Docker build context is the `environment/` directory, **not** the task root directory.
  - **CORRECT**: `COPY app /app` and `COPY tests /app/tests`
  - **INCORRECT (Build Fails)**: `COPY environment/app /app` or `COPY environment/tests /app/tests` (causes `"/environment/tests": not found`).
- **Base Image & Package Availability**:
  - Prefer `FROM ubuntu:24.04` (Noble) for C/C++/Systems tasks (natively includes GCC 13, Clang 18, LLVM, CMake, Ninja, and Python 3.12 without broken `update-alternatives` or missing PPA errors).
  - Use `FROM python:3.11-slim` for pure Python APIs.
  - Never specify nonexistent packages on older Ubuntu versions (e.g., `gcc-13` on `ubuntu:22.04` causes `apt-get install` failure).

### 2. Mandatory Verifier Reward Files (`verifier/reward.txt` & `reward.json`)
- **Reward File Rule**: Every trial (both untouched **NOP** starter code and **Oracle** solution runs) must produce a reward file. Failing to write this file results in `Your verifier completed without writing a reward file (verifier/reward.txt or reward.json)`.
- **Implementation**:
  - `tests/test.sh` must write the floating-point score (e.g., `0.0` or `1.0`) to `verifier/reward.txt`, `reward.txt`, `/tmp/verifier/reward.txt`, and `/tmp/reward.txt`.
  - It must write `{"reward": <score>, "passed": [...]}` to `reward.json` and `verifier/reward.json`.
  - Create parent directories (`mkdir -p verifier /tmp/verifier /logs/verifier`) to guarantee file creation regardless of container working directory.
  - Discover `hidden_verifier.py` dynamically across `/tests/hidden_verifier.py`, `tests/hidden_verifier.py`, and `/app/tests/hidden_verifier.py`.

### 3. Oracle Solution Protocol Exactness (`The reference solution didn't solve the task`)
- **Response Format Rule**: When implementing API endpoints, verify top-level payload schemas strictly against RFCs and verifier assertions.
  - For example, in FastAPI, raising `HTTPException(status_code=400, detail={"error": "invalid_grant"})` nests the error under `{"detail": {"error": ...}}`.
  - To return top-level RFC-compliant JSON (`{"error": "invalid_grant"}`), always use `JSONResponse(status_code=400, content={"error": "invalid_grant"})`.
- **Local Pre-Verification**: Always test `solution/solve.sh` against the hidden verifier suite to guarantee a 100% full-reward run.

### 4. Verifier Family Synchronization (`verifier family mismatch`)
- The `verifierFamily` field selected in the web draft form must match `verifier_family` in `task.toml` exactly:
  - If using `test.sh` unit test scripts: use `"programmatic"` (both in the draft form and in `task.toml`).

### 5. ZIP Path Delimiters (`Bundle rejected: unsafe path`)
- Windows backslashes (`\`) inside ZIP entry paths trigger immediate rejection by Odyssey quarantine scanners.
- Always use a Python script with `posix_name = rel_path.replace("\\", "/")` to write archives with forward-slash delimiters exclusively.

### 6. Difficulty Probe & Long-Horizon Scope Floor (`Too short for the collection — not long-horizon`)
- **Root Cause**: Odyssey evaluates tasks using autonomous difficulty probe agents. If a task consists of only 1–2 simple endpoints or <100 lines of straightforward logic, automated frontier agents solve it in a single trivial turn (10–15 minutes), causing immediate rejection at the **Difficulty evaluation** stage.
- **Requirements to Pass Difficulty Evaluation**:
  - **Substantial Scope**: Tasks must represent comprehensive systems (e.g., 6–10 interrelated endpoints/features, full RFC state machines, asymmetric cryptography, token lifecycle management, database transactions).
  - **Expert Effort & Compute Budget**:
    - Set `expertTimeEstimateHours`: 16 – 20 hours.
    - Set `agentTimeoutSec`: 28800 (8 hours) to reflect true long-horizon evaluation.
    - Set `verifierTimeoutSec`: 1800 (30 minutes).
  - **Multi-Module Depth**: Separate concerns across data models, crypto/hash engines, background workers, storage tables, and protocol validation.

### 7. Rubric Review & 4-Way Spec Alignment Guarantee (`The task didn't meet the bar in an automated validation check`)
- **Root Cause**: The Odyssey Rubric Review stage employs an automated LLM judge that performs an exact 4-way consistency and quality evaluation across:
  1. **Draft Metadata** (`drafts/*.json`)
  2. **Problem Statement** (`instruction.md`)
  3. **Starter Environment** (`environment/app/` — `main.py`, `models.py`, `database.py`, and `environment/tests/public_test.py`)
  4. **Sealed Verifier & Reference Solution** (`tests/hidden_verifier.py` and `solution/solve.sh`)
- **Requirements to Pass Rubric Review**:
  - **Complete Starter Stubs**: Every single endpoint mentioned in `instruction.md` must exist as a typed stub in `environment/app/main.py` (raising `HTTPException(status_code=501)` or returning a starter response). Never leave route paths absent from `main.py`.
  - **Synchronized Schema & Models**: All database tables (`sagas`, `steps`, `journal`, etc.) and Pydantic models must be fully declared in `environment/app/database.py` and `environment/app/models.py`.
  - **Meaningful Public Smoke Tests**: `environment/tests/public_test.py` must perform actual schema and route table sanity checks (e.g. asserting all declared API routes exist in `app.routes`), rather than trivial 1-line stubs.
  - **Clean JSON Formatting**: Ensure `title` and all string fields in `drafts/*.json` are clean, well-formatted strings without stray trailing quotes or unescaped characters.

### 8. Trial Envelope Ceiling & Agent Timeout Limits (`Above 37000s (~10h)`)
- **Root Cause**: The total per-trial execution ceiling (Build + Agent + Verify + Teardown) is strictly capped at **14 hours (50,400 seconds)** across all phases.
- **Rules & Recommended Bounds**:
  - **`agentTimeoutSec`**: Must be **$\le 36,000$ seconds (10 hours)**. The standard recommended setting for long-horizon tasks is **`28800` seconds (8 hours)**.
  - Setting `agentTimeoutSec: 43200` (12h) causes an intake validation error: *"Above 37000s (~10h) — leave room for build, verify, teardown, which share a trial's 14h wall-clock limit"*.
  - **`verifierTimeoutSec`**: Set to **`1800` seconds (30 minutes)**.
  - **`expertTimeEstimateHours`**: Set between **`16` and `20` hours** (this is a human effort estimate and is independent of the agent execution timeout).
