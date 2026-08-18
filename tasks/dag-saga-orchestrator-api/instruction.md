# Enterprise Distributed DAG Saga Orchestrator API

## Overview
Your objective is to implement an enterprise-grade Distributed DAG (Directed Acyclic Graph) Saga Orchestration Engine in Python/FastAPI using SQLite and AsyncIO in `/app`.

A Saga is defined as an arbitrary Directed Acyclic Graph (DAG) of distributed steps. Each step defines an `execute_url`, a `compensate_url`, a `payload`, a list of `dependencies` (IDs of prerequisite steps), and an optional `retry_policy` (`max_retries`, `backoff_sec`, `timeout_sec`).

## Core Requirements

### 1. Saga Ingestion, Graph Validation & Cycle Detection
- Endpoint: `POST /api/v1/sagas`
  - Ingests `saga_id` and list of `steps`.
  - Optional `Idempotency-Key` header: Repeated requests with the same key must return the existing saga without re-execution.
  - **Validation:**
    - Reject dangling dependencies (references to step IDs not defined in the saga) with `400 Bad Request` and `{"error": "invalid_dependency"}`.
    - Run Kahn's algorithm or DFS cycle detection. Reject cyclic graphs with `400 Bad Request` and `{"error": "cyclic_dependency_detected"}`.
  - Returns `202 Accepted` with `{"saga_id": "<id>", "status": "running"}` and schedules asynchronous background execution.

### 2. Concurrent Forward Topological Execution & Retries
- Concurrently dispatch all frontier nodes with zero unmet dependencies via HTTP POST to `execute_url`.
- **Retries & Timeouts:**
  - If a step responds with transient HTTP 5xx or times out (`timeout_sec`), retry up to `max_retries` with exponential backoff (`backoff_sec`).
  - If all retries are exhausted, mark the step as `failed` and initiate compensation.
- When all steps succeed, transition saga status to `completed`.

### 3. Reverse Topological Parallel Compensation
- When any step fails unrecoverably, transition saga status to `compensating`.
- Compute the reverse topological dependency graph of all completed steps.
- Compensate completed leaf steps first via HTTP POST to `compensate_url`. Independent branches in the compensation graph must execute in parallel.
- When all compensations finish, transition saga status to `compensated`.

### 4. Saga Lifecycle, Controls & Journal Audit
- Endpoint: `GET /api/v1/sagas/{saga_id}`: Returns `{"saga_id": "<id>", "status": "<status>", "created_at": ..., "updated_at": ...}` (or `404 Not Found`).
- Endpoint: `GET /api/v1/sagas/{saga_id}/steps`: Returns step-level statuses (`pending`, `executing`, `completed`, `failed`, `compensating`, `compensated`).
- Endpoint: `GET /api/v1/sagas/{saga_id}/journal`: Returns the ordered, append-only SQLite state transition journal entries.
- Endpoint: `POST /api/v1/sagas/{saga_id}/pause`: Pauses in-flight scheduling.
- Endpoint: `POST /api/v1/sagas/{saga_id}/resume`: Resumes paused saga execution.

## Verification
Run public tests via `pytest /app/tests/public_test.py`.
The hidden verifier tests cycle detection, forward topological parallel execution on wide diamond DAGs, retry backoff on transient errors, parallel reverse leaf compensation, pause/resume controls, and database journal integrity.
