# Problem Statement: Distributed DAG Saga Orchestrator API

Implement a persistent Saga Orchestrator API using FastAPI and SQLite in `/app`.

In distributed systems, traditional database transactions (Two-Phase Commits) don't work across microservices. Instead, we use the **Saga Pattern**. 

## Core Requirements

### 1. Saga Creation
- **Endpoint:** `POST /api/v1/sagas`
- **Behavior:** Accepts a JSON body defining a `saga_id` and an array of `steps` (a Directed Acyclic Graph). 
- Validate that no step lists a dependency that doesn't exist in the payload. If invalid, return `400 Bad Request`.
- Persist the saga and steps in SQLite (`sagas` and `steps` tables).
- Kick off execution asynchronously in the background. Return `202 Accepted` with `{"saga_id": "...", "status": "running"}`.

### 2. Forward DAG Execution
- Use `httpx.AsyncClient` to dispatch the webhooks.
- Steps with empty `dependencies` execute immediately.
- A step can ONLY execute once ALL of the step IDs listed in its `dependencies` array have reached a `completed` state.
- Execution is an HTTP POST to the step's `execute_url` with its `payload`. 
- If the HTTP call returns 2xx, mark the step `completed` in SQLite.
- If all steps complete, mark the saga `completed`.

### 3. Failure & Reverse Compensation (The Hard Part)
- If an `execute_url` returns a non-2xx status code, mark that step as `failed`.
- **Halt forward progress:** Do not execute any further pending steps.
- **Compensate:** You must immediately begin undoing the saga by making HTTP POSTs to the `compensate_url` (with an empty JSON body `{}`) for steps that succeeded.
- **CRITICAL:** You must ONLY compensate steps that successfully `completed`. 
- **CRITICAL:** Compensations MUST execute in strict **reverse-topological order**. If Step B depends on Step A, you must fully compensate Step B *before* starting the compensation for Step A.
- Once finished, the saga status becomes `compensated`.

### 4. Status Check
- **Endpoint:** `GET /api/v1/sagas/{saga_id}`
- Returns `{"saga_id": "...", "status": "..."}`. Statuses must be one of: `running`, `completed`, `compensating`, `compensated`.

## Verification
You can run public checks via `pytest /app/tests/public_test.py`. 
The hidden grading suite will construct complex branching DAGs, simulate webhook failures via a mock server, and strictly assert the chronological order of your HTTP webhook requests to ensure reverse-topological compliance.