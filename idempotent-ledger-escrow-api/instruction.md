# Problem Statement: Idempotent Multi-Tenant Escrow & Double-Entry Ledger API

You are tasked with building a high-reliability, multi-tenant Ledger and Escrow REST API using **FastAPI** and **SQLite3** in `/app`.

The application skeleton is in `/app`, but the core business logic in `/app/main.py` and `/app/models.py` consists of empty stubs returning `501 Not Implemented`.

## System Requirements

### 1. Data Integrity & Currency Representation
- All monetary amounts must be stored as 64-bit integer minor units (e.g., cents: `$10.50` = `1050`). No floating-point balances.
- Balances can never drop below zero.
- **Available Balance** = `Current Balance - Active Unexpired Holds`.
- Every committed transfer must create balanced ledger entries ($\sum \text{debits} - \sum \text{credits} = 0$).

### 2. Idempotency (`Idempotency-Key` header)
All state-mutating endpoints (`POST /api/v1/accounts`, `POST /api/v1/transfers`, `POST /api/v1/holds`, `POST /api/v1/holds/{id}/capture`, `POST /api/v1/holds/{id}/void`) accept an optional or required `Idempotency-Key` header.
- **First request:** Executes normally, stores key + SHA-256 hash of the request body + status code + response body.
- **Identical retry (same key, same body):** Returns the cached HTTP status code and response body.
- **Conflicting retry (same key, DIFFERENT body):** Immediately returns `422 Unprocessable Entity` with `{"error": "Idempotency conflict"}`.

### 3. Escrow & Authorization Holds
- `POST /api/v1/holds`: Creates an authorization hold reserving funds from a sender account. 
  - Requires: `account_id`, `amount`, `currency`, `expires_at` (ISO-8601 UTC string).
  - Fails with `400 Bad Request` if `amount > available_balance`.
  - State becomes `active`.
- `POST /api/v1/holds/{id}/capture`: Settles the hold.
  - Accepts optional `capture_amount` ($\le \text{held amount}$).
  - If `capture_amount < held_amount`, the remaining amount (`held_amount - capture_amount`) is automatically released back to the sender's available balance.
  - If hold is already expired, captured, or voided, return `409 Conflict`.
- `POST /api/v1/holds/{id}/void`: Cancels the hold and releases all held funds.
- **Auto-Expiration:** Any access or transaction involving an account with holds where `utc_now() > expires_at` must treat those holds as `expired` and release the reserved funds.

### 4. Admin HMAC-SHA256 Authentication
Administrative endpoints (e.g., `POST /api/v1/admin/reconcile`) require:
- Header `X-Timestamp`: Unix epoch timestamp in seconds.
- Header `X-Signature-SHA256`: Hex-encoded HMAC-SHA256 signature calculated over `f"{timestamp}.{raw_request_body}"` using the tenant's secret key.
- Reject requests with `401 Unauthorized` if timestamp skew exceeds 300 seconds from current server time or signature is invalid.

## Verification
You can run the public test suite anytime:
```bash
pytest /app/tests/public_test.py