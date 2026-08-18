# Idempotent Multi-Tenant Escrow & Double-Entry Ledger API

## Overview
Your objective is to implement an enterprise multi-tenant financial escrow engine and double-entry bookkeeping ledger in Python/FastAPI using SQLite in `/app`.

## Core Requirements

### 1. Account Management & Initial Balances
- `POST /api/v1/accounts` (Status `201 Created`):
  - Ingests `id` (optional), `tenant_id`, `currency`, and `initial_balance` (int).
  - If `initial_balance > 0`, records a balanced transaction with matching debit and credit entries in `ledger_entries`.
  - Returns `{"id": "...", "tenant_id": "...", "currency": "...", "balance": <int>, "available_balance": <int>}`.
- `GET /api/v1/accounts/{account_id}`:
  - Returns current account details with `balance` (total funds) and `available_balance` (total funds minus active holds). Returns `404 Not Found` if missing.

### 2. Idempotent Transfers (Zero-Sum Double-Entry)
- `POST /api/v1/transfers`:
  - Ingests `from_account_id`, `to_account_id`, and `amount` (int > 0).
  - Headers: `Idempotency-Key` (optional string) and `X-Tenant-ID` (optional, defaults to `"default"`).
  - **Idempotency Semantics:**
    - If `Idempotency-Key` is provided, compute SHA-256 digest of raw request body.
    - If key was seen before with identical body hash, return the cached HTTP response and body without re-executing.
    - If key was seen before with a different body hash, return `422 Unprocessable Entity` with `{"error": "Idempotency conflict"}`.
  - **Transfer Invariant:** Deduct `amount` from `from_account_id` and credit `amount` to `to_account_id`. Insert balanced debit (`-amount`) and credit (`+amount`) rows in `ledger_entries`.

### 3. Escrow Holds, Capture & Void
- `POST /api/v1/holds` (Status `201 Created`):
  - Ingests `account_id`, `amount`, `currency`, and `expires_at` (ISO 8601 UTC timestamp).
  - Verifies `amount <= available_balance`. Places funds in hold state `active` (reducing `available_balance` without deducting from total `balance`).
- `POST /api/v1/holds/{hold_id}/capture`:
  - Ingests `destination_account_id` and optional `capture_amount` (defaults to full hold amount).
  - Validates `hold.status == "active"` and `expires_at > now`. If expired or already settled, returns `409 Conflict`.
  - Deducts `capture_amount` from holder account and credits destination account, creating balanced entries in `ledger_entries`. Sets hold status to `captured`. Any uncaptured remainder is automatically released back to available balance.
- `POST /api/v1/holds/{hold_id}/void`:
  - Validates `hold.status == "active"`. Sets status to `voided`, restoring available balance. Returns `409 Conflict` if not active.

### 4. Lazy Temporal Auto-Expiration
- When querying balances or processing holds, any hold where `expires_at <= now` and `status == "active"` must be lazily marked as `expired` or excluded from active hold sums, immediately releasing available balance.

### 5. Administrative HMAC-SHA256 Reconciliation
- `POST /api/v1/admin/reconcile`:
  - Headers: `X-Timestamp` (UNIX epoch seconds) and `X-Signature-SHA256` (hex HMAC-SHA256 signature).
  - Signed payload format: `f"{X-Timestamp}.".encode() + raw_body`.
  - Shared secret: `b"test_secret_key_12345"`.
  - Rejects missing headers (`401`), clock skew `|now - timestamp| > 300s` (`401`), and invalid signatures (`401`).

## Verification
Public smoke tests can be run via `pytest /app/tests/public_test.py`.
The sealed verifier asserts all 6 financial invariants, zero-sum double-entry consistency, and HMAC verification.
