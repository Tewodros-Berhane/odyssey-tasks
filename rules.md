# Odyssey Task Authoring Rules & Guidelines

## 1. Overview & Two-Phase Submission Flow
Odyssey collects self-contained software-engineering tasks solved autonomously by frontier coding agents and graded objectively by sealed verifiers. Authoring follows a two-phase flow:
1. **Create Draft**: Complete all structured metadata fields (title, objective, verification strategy, resource limits, etc.). Drafts are versioned and can be iterated on.
2. **Request Upload URL**: Obtain a short-lived, signed URL pointing to a private quarantine bucket.
3. **Upload Task Bundle**: Upload a single `.zip` archive (up to 512 MiB compressed). Artifact lands in an `uploaded` state.
4. **Automated Quarantine & Inspection**: Archive is inspected in isolation for structure, path safety, and integrity (`inspecting` → `safe` or `rejected`).
5. **Automatic Submission**: When marked `safe`, the system snapshots draft + bundle into a submission and triggers the automated pipeline.
6. **Deduplication**: Exact byte-for-byte duplicate bundles are blocked by content hash; near-duplicates are caught by embedding-based similarity checks.

---

## 2. Draft Metadata Fields & Constraints

### Task Identity
- **`title`** (3–200 chars): Short human-readable name for reviewers skimming the queue (not for the agent).
- **`workingSlug`** (3–80 chars, lowercase-kebab: `^[a-z0-9-]+$`): URL-safe draft identifier (e.g., `parse-toml-strict`).
- **`collectionFamily`** (enum, locked choice):
  - `Library clone` (reimplement a focused library/module to spec)
  - `Product clone` (build a working slice of a real application)
  - `ML engineering` (train, tune, or wire up a model against a metric)
  - `Algorithmic optimization` (make a correct solution measurably faster/leaner)
- **`taskFamily`** (enum): `feature_development`, `debugging`, `refactoring`, `performance`, `systems_integration`, or `other`.
- **`verifierFamily`** (enum): `programmatic` (tests/scripts), `optimization` (metric to push), `ml_artifact` (trained artifact), or `custom`.

### Task Definition
- **`objective`** (40–20,000 chars): Concrete deliverable handed to an engineer; describes what the agent builds/fixes and defines "done".
- **`motivation`** (20–10,000 chars): Real-world scenario and capability grounding why this task is worth grading.

### Difficulty & Effort
- **`difficultyExplanation`** (40–20,000 chars): Specific technical breakdown of difficulty, traps, reasoning bottlenecks, and why frontier models fail to one-shot it.
- **`expertTimeEstimateHours`** (positive number): Descriptive estimate of end-to-end human expert completion time (not an agent gate).

### Environment & Resources
- **`environmentSummary`** (40–20,000 chars): Base image, languages, tooling, pre-installed dependencies, and initial state in `/app`. Everything must be pre-baked.
- **`resourceEstimate`** (structured):
  - `cpuMillis`: 100 – 64,000 (Sandbox ceiling: 8 CPUs)
  - `memoryMb`: 128 – 262,144 (Sandbox ceiling: 65,536 MB)
  - `storageMb`: 128 – 1,048,576 (Sandbox ceiling: 40,960 MB)
  - `gpuCount`: 0 – 8
  - `agentTimeoutSec`: 7,200 – 86,400 (Minimum long-horizon floor: 2h / 7,200s; cap: 24h / 86,400s)
  - `verifierTimeoutSec`: 1 – 86,400
  - **Trial ceiling**: Build + Agent + Verify + Teardown must fit within 50,400s (14h) total pool ceiling. Requests above physical limits are rejected at intake.
- **`networkRequirements`** (structured):
  - Mode: `none` (default, fully offline — preferred for determinism) or `allowlist` (1–100 specific hosts).
  - Open/unrestricted internet egress is strictly rejected.
  - Requires non-empty justification if `allowlist` is selected.

### Oracle & Verification
- **`oracleStrategy`** (20–20,000 chars): Details how reference solution under `solution/` solves the task to achieve 100% reward.
- **`verificationStrategy`** (40–20,000 chars): Explanation of how `tests/` objectively evaluates success, prevents false passes, and defines the visible vs. hidden verifier split.

### Scoring & Exploits
- **`binarySuccessCondition`** (20–10,000 chars): Unambiguous, machine-checkable pass/fail criteria for complete resolution.
- **`partialScoreStrategy`** (20–10,000 chars): Continuous, monotone scoring breakdown and reward components for partial progress.
- **`anticipatedExploits`** (20–20,000 chars): Anticipated model shortcuts (hard-coding, test leakage, metric gaming) and how the verifier mitigates each.

---

## 3. Task Bundle Structure & File Requirements

The bundle must be a single ZIP archive (≤ 512 MiB compressed) laid out in the Terminal-Bench / Harbor task format:

```text
<task-dir>/
├── task.toml            # [metadata], [verifier], [agent], [environment]
├── instruction.md       # Problem statement read by the agent
├── environment/
│   └── Dockerfile       # Builds /app with all dependencies baked in (no runtime net)
├── tests/
│   └── test.sh          # Canonical sealed verifier entrypoint (grader + held-out data)
└── solution/
    └── solve.sh         # Canonical reference solution entrypoint run by oracle
```

### Bundle Validation Rules:
1. **`task.toml`**: Must be well-formed TOML containing valid `[metadata]`, `[environment]`, `[agent]`, and `[verifier]` sections.
   - Resource values in `task.toml` must be ≤ values declared in the draft form.
   - Network mode must be declared per phase (`[environment]`, `[agent]`, `[verifier]`).
   - If `[metadata].open_internet_justification` is present, `[agent].network_mode` must be explicitly declared (either `none` or `allowlist`; `open` is rejected).
2. **`instruction.md`**: Must contain comprehensive, substantive problem instructions (not placeholders or stubs).
3. **`environment/Dockerfile`**: Must build the complete `/app` environment without requiring runtime network access.
4. **`tests/test.sh`**: Required canonical script name for grading.
5. **`solution/solve.sh`**: Required canonical script name for oracle execution.

---

## 4. Automated Evaluation Funnel Stages
All submitted tasks run through automated pipeline stages before reaching human reviewers:

1. **Structure Stage (Deterministic)**:
   - Validates existence and paths of required files (`task.toml`, `instruction.md`, `environment/Dockerfile`, `tests/test.sh`, `solution/solve.sh`).
   - Ensures valid TOML syntax and path safety.
2. **Similarity & Deduplication Stage**:
   - Runs corpus embedding search to block near-duplicate or reskinned tasks.
3. **Oracle & NOP Stage**:
   - **Oracle Run**: Executes `solution/solve.sh`; must achieve near 100% full reward.
   - **NOP Run**: Runs on untouched initial state; must score at baseline floor (0%).
4. **Quality Check Stage**:
   - Verifies substantive instruction content, non-empty metadata, and presence of verifier/solution scripts.
   - Evaluates bundle via an injection-hardened LLM judge against clarity, completeness, anti-gaming adequacy, and verifier alignment.
5. **Difficulty Probe Stage**:
   - Runs independent frontier agent trials across full time budgets to ensure the task cannot be trivially one-shot solved and is not unsolvable.
6. **Synthesis Stage**:
   - Terminal stage aggregating previous results and producing the final automated verdict.
   - Verdict failures return actionable reasons; platform/infra flakes are re-run without penalty.

---

## 5. Human Review & Acceptance Bar

To clear final human review and qualify for payout, tasks must meet these criteria:
- **True Family Fit**: Belongs unambiguously to one of the four collection families (`Library clone`, `Product clone`, `ML engineering`, `Algorithmic optimization`).
- **Oracle Solvable**: Reference solution reliably achieves full reward on the verifier.
- **Multi-Channel Robust Verification**: Tests behavior, invariants, edge cases, and performance from multiple independent angles—never relying on single assertions.
- **Visible / Hidden Verifier Split**: Provides public test cases for self-checking while keeping decisive evaluation sets and grading logic sealed.
- **Resistant to Exploits**: Defeats hard-coding, metric gaming, or leakage of held-out datasets.
- **Realistic & Grounded**: Mirrors genuine software engineering workflows, tools, and environments rather than artificial drills.
- **Novel**: Truly original problem formulation, not a re-skin of existing benchmark exercises.
- **Calibrated Difficulty**: Proven solvable by oracle while resisting trivial saturation by frontier models.

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
