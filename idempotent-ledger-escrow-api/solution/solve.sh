#!/usr/bin/env bash
set -e

cat << 'EOF' > /app/main.py
import hashlib
import hmac
import json
import time
import uuid
from datetime import datetime, timezone
from typing import Optional
from fastapi import FastAPI, Header, HTTPException, Request, Response
from pydantic import BaseModel
from database import get_db, init_db

app = FastAPI(title="Idempotent Escrow Ledger API")
ADMIN_SECRET = b"test_secret_key_12345"

@app.on_event("startup")
def startup():
    init_db()

def expire_stale_holds(conn):
    now_iso = datetime.now(timezone.utc).isoformat()
    conn.execute("UPDATE holds SET status = 'expired' WHERE status = 'active' AND expires_at < ?", (now_iso,))

def get_account_balances(conn, account_id: str):
    expire_stale_holds(conn)
    acc = conn.execute("SELECT * FROM accounts WHERE id = ?", (account_id,)).fetchone()
    if not acc:
        return None
    hold_sum = conn.execute("SELECT COALESCE(SUM(amount), 0) as h FROM holds WHERE account_id = ? AND status = 'active'", (account_id,)).fetchone()["h"]
    avail = acc["balance"] - hold_sum
    return acc, avail

@app.post("/api/v1/accounts", status_code=201)
async def create_account(req: Request):
    data = await req.json()
    acc_id = data.get("id", f"acc_{uuid.uuid4().hex[:12]}")
    tenant_id = data["tenant_id"]
    currency = data["currency"]
    balance = data.get("initial_balance", 0)

    conn = get_db()
    with conn:
        conn.execute("INSERT INTO accounts (id, tenant_id, currency, balance) VALUES (?, ?, ?, ?)", (acc_id, tenant_id, currency, balance))
        if balance > 0:
            tx_id = f"tx_init_{uuid.uuid4().hex[:8]}"
            conn.execute("INSERT INTO transactions (id, tenant_id, reference) VALUES (?, ?, 'initial_deposit')", (tx_id, tenant_id))
            conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, acc_id, balance))
            conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, acc_id, -balance))
    
    return {"id": acc_id, "tenant_id": tenant_id, "currency": currency, "balance": balance, "available_balance": balance}

@app.get("/api/v1/accounts/{account_id}")
def get_account(account_id: str):
    conn = get_db()
    with conn:
        res = get_account_balances(conn, account_id)
        if not res:
            raise HTTPException(status_code=404, detail="Account not found")
        acc, avail = res
        return {"id": acc["id"], "tenant_id": acc["tenant_id"], "currency": acc["currency"], "balance": acc["balance"], "available_balance": avail}

@app.post("/api/v1/transfers")
async def transfer(req: Request, idempotency_key: Optional[str] = Header(None, alias="Idempotency-Key"), x_tenant_id: Optional[str] = Header("default", alias="X-Tenant-ID")):
    body_raw = await req.body()
    data = json.loads(body_raw)
    conn = get_db()

    with conn:
        if idempotency_key:
            req_hash = hashlib.sha256(body_raw).hexdigest()
            row = conn.execute("SELECT * FROM idempotency_keys WHERE key = ? AND tenant_id = ?", (idempotency_key, x_tenant_id)).fetchone()
            if row:
                if row["request_hash"] != req_hash:
                    raise HTTPException(status_code=422, detail="Idempotency conflict")
                return Response(content=row["response_body"], status_code=row["response_code"], media_type="application/json")

        from_acc, avail = get_account_balances(conn, data["from_account_id"])
        to_acc = conn.execute("SELECT * FROM accounts WHERE id = ?", (data["to_account_id"],)).fetchone()
        amt = data["amount"]

        if not from_acc or not to_acc or avail < amt or amt <= 0:
            raise HTTPException(status_code=400, detail="Invalid transfer amount or insufficient funds")

        conn.execute("UPDATE accounts SET balance = balance - ? WHERE id = ?", (amt, from_acc["id"]))
        conn.execute("UPDATE accounts SET balance = balance + ? WHERE id = ?", (amt, to_acc["id"]))

        tx_id = f"tx_{uuid.uuid4().hex[:8]}"
        conn.execute("INSERT INTO transactions (id, tenant_id, reference) VALUES (?, ?, 'transfer')", (tx_id, x_tenant_id))
        conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, from_acc["id"], -amt))
        conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, to_acc["id"], amt))

        resp_data = {"status": "success", "transaction_id": tx_id, "amount": amt}
        resp_json = json.dumps(resp_data)

        if idempotency_key:
            conn.execute("INSERT INTO idempotency_keys (key, tenant_id, request_hash, response_code, response_body) VALUES (?, ?, ?, 200, ?)",
                         (idempotency_key, x_tenant_id, req_hash, resp_json))

        return resp_data

@app.post("/api/v1/holds", status_code=201)
async def create_hold(req: Request):
    data = await req.json()
    conn = get_db()
    with conn:
        res = get_account_balances(conn, data["account_id"])
        if not res:
            raise HTTPException(status_code=404, detail="Account not found")
        acc, avail = res
        if data["amount"] > avail or data["amount"] <= 0:
            raise HTTPException(status_code=400, detail="Insufficient available balance for hold")

        hold_id = f"hold_{uuid.uuid4().hex[:8]}"
        conn.execute("INSERT INTO holds (id, account_id, amount, currency, status, expires_at) VALUES (?, ?, ?, ?, 'active', ?)",
                     (hold_id, acc["id"], data["amount"], data["currency"], data["expires_at"]))

    return {"id": hold_id, "status": "active", "amount": data["amount"], "expires_at": data["expires_at"]}

@app.post("/api/v1/holds/{hold_id}/capture")
async def capture_hold(hold_id: str, req: Request):
    data = await req.json()
    conn = get_db()
    with conn:
        expire_stale_holds(conn)
        hold = conn.execute("SELECT * FROM holds WHERE id = ?", (hold_id,)).fetchone()
        if not hold:
            raise HTTPException(status_code=404, detail="Hold not found")
        if hold["status"] != "active":
            raise HTTPException(status_code=409, detail=f"Hold cannot be captured in status: {hold['status']}")

        capture_amt = data.get("capture_amount", hold["amount"])
        if capture_amt > hold["amount"] or capture_amt <= 0:
            raise HTTPException(status_code=400, detail="Invalid capture amount")

        dest_acc_id = data["destination_account_id"]
        conn.execute("UPDATE accounts SET balance = balance - ? WHERE id = ?", (capture_amt, hold["account_id"]))
        conn.execute("UPDATE accounts SET balance = balance + ? WHERE id = ?", (capture_amt, dest_acc_id))
        conn.execute("UPDATE holds SET status = 'captured' WHERE id = ?", (hold_id,))

        tx_id = f"tx_cap_{uuid.uuid4().hex[:8]}"
        conn.execute("INSERT INTO transactions (id, tenant_id, reference) VALUES (?, 'system', 'hold_capture')", (tx_id,))
        conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, hold["account_id"], -capture_amt))
        conn.execute("INSERT INTO ledger_entries (id, transaction_id, account_id, amount) VALUES (?, ?, ?, ?)", (str(uuid.uuid4()), tx_id, dest_acc_id, capture_amt))

    return {"status": "captured", "captured_amount": capture_amt}

@app.post("/api/v1/holds/{hold_id}/void")
def void_hold(hold_id: str):
    conn = get_db()
    with conn:
        expire_stale_holds(conn)
        hold = conn.execute("SELECT * FROM holds WHERE id = ?", (hold_id,)).fetchone()
        if not hold or hold["status"] != "active":
            raise HTTPException(status_code=409, detail="Hold cannot be voided")
        conn.execute("UPDATE holds SET status = 'voided' WHERE id = ?", (hold_id,))
    return {"status": "voided"}

@app.post("/api/v1/admin/reconcile")
async def admin_reconcile(req: Request, x_timestamp: Optional[str] = Header(None, alias="X-Timestamp"), x_signature_sha256: Optional[str] = Header(None, alias="X-Signature-SHA256")):
    if not x_timestamp or not x_signature_sha256:
        raise HTTPException(status_code=401, detail="Missing auth headers")
    
    try:
        ts = int(x_timestamp)
    except ValueError:
        raise HTTPException(status_code=401, detail="Invalid timestamp")

    if abs(time.time() - ts) > 300:
        raise HTTPException(status_code=401, detail="Timestamp skew exceeded")

    body_bytes = await req.body()
    expected_sig = hmac.new(ADMIN_SECRET, f"{ts}.".encode() + body_bytes, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected_sig, x_signature_sha256):
        raise HTTPException(status_code=401, detail="Invalid HMAC signature")

    return {"reconciliation": "ok", "timestamp": ts}
EOF

echo "Reference solution deployed successfully."