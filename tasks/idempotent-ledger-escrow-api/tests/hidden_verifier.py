import pytest
import hmac
import hashlib
import time
import uuid
from datetime import datetime, timedelta, timezone
from fastapi.testclient import TestClient
from main import app
from database import init_db, get_db

client = TestClient(app)

@pytest.fixture(autouse=True)
def fresh_db():
    init_db()

def test_crud_and_initialization():
    acc_id = f"acc_{uuid.uuid4().hex[:8]}"
    r = client.post("/api/v1/accounts", json={"id": acc_id, "tenant_id": "tenant_a", "currency": "USD", "initial_balance": 5000})
    assert r.status_code == 201
    data = r.json()
    assert data["balance"] == 5000
    assert data["available_balance"] == 5000

    r_get = client.get(f"/api/v1/accounts/{acc_id}")
    assert r_get.status_code == 200
    assert r_get.json()["available_balance"] == 5000

def test_idempotency_and_hash_conflicts():
    acc_1 = f"acc_{uuid.uuid4().hex[:8]}"
    acc_2 = f"acc_{uuid.uuid4().hex[:8]}"
    client.post("/api/v1/accounts", json={"id": acc_1, "tenant_id": "t1", "currency": "USD", "initial_balance": 10000})
    client.post("/api/v1/accounts", json={"id": acc_2, "tenant_id": "t1", "currency": "USD", "initial_balance": 0})

    idem_key = f"idem_{uuid.uuid4().hex}"
    payload1 = {"from_account_id": acc_1, "to_account_id": acc_2, "amount": 2000}
    
    # First execution
    r1 = client.post("/api/v1/transfers", json=payload1, headers={"Idempotency-Key": idem_key, "X-Tenant-ID": "t1"})
    assert r1.status_code == 200

    # Identical replay
    r2 = client.post("/api/v1/transfers", json=payload1, headers={"Idempotency-Key": idem_key, "X-Tenant-ID": "t1"})
    assert r2.status_code == 200
    assert r2.json() == r1.json()

    # Conflicting body with same key -> MUST be 422
    payload2 = {"from_account_id": acc_1, "to_account_id": acc_2, "amount": 9999}
    r3 = client.post("/api/v1/transfers", json=payload2, headers={"Idempotency-Key": idem_key, "X-Tenant-ID": "t1"})
    assert r3.status_code == 422

def test_escrow_hold_capture_partial_and_void():
    acc_1 = f"acc_{uuid.uuid4().hex[:8]}"
    acc_2 = f"acc_{uuid.uuid4().hex[:8]}"
    client.post("/api/v1/accounts", json={"id": acc_1, "tenant_id": "t1", "currency": "USD", "initial_balance": 10000})
    client.post("/api/v1/accounts", json={"id": acc_2, "tenant_id": "t1", "currency": "USD", "initial_balance": 0})

    exp = (datetime.now(timezone.utc) + timedelta(hours=1)).isoformat()
    r_hold = client.post("/api/v1/holds", json={
        "account_id": acc_1,
        "amount": 4000,
        "currency": "USD",
        "expires_at": exp
    })
    assert r_hold.status_code == 201
    hold_id = r_hold.json()["id"]

    # Check available balance dropped to 6000 while total balance remains 10000
    r_acc = client.get(f"/api/v1/accounts/{acc_1}")
    assert r_acc.json()["balance"] == 10000
    assert r_acc.json()["available_balance"] == 6000

    # Partial capture of 2500 -> 1500 released back to acc_1
    r_cap = client.post(f"/api/v1/holds/{hold_id}/capture", json={
        "destination_account_id": acc_2,
        "capture_amount": 2500
    })
    assert r_cap.status_code == 200

    r_acc1_after = client.get(f"/api/v1/accounts/{acc_1}")
    assert r_acc1_after.json()["balance"] == 7500
    assert r_acc1_after.json()["available_balance"] == 7500

    r_acc2_after = client.get(f"/api/v1/accounts/{acc_2}")
    assert r_acc2_after.json()["balance"] == 2500

    # Second capture on settled hold must 409
    r_double_cap = client.post(f"/api/v1/holds/{hold_id}/capture", json={"destination_account_id": acc_2, "capture_amount": 100})
    assert r_double_cap.status_code == 409

def test_auto_expiration_and_temporal_release():
    acc_1 = f"acc_{uuid.uuid4().hex[:8]}"
    client.post("/api/v1/accounts", json={"id": acc_1, "tenant_id": "t1", "currency": "USD", "initial_balance": 5000})

    # Hold with expiration in past
    past_exp = (datetime.now(timezone.utc) - timedelta(seconds=10)).isoformat()
    r_hold = client.post("/api/v1/holds", json={
        "account_id": acc_1,
        "amount": 3000,
        "currency": "USD",
        "expires_at": past_exp
    })
    assert r_hold.status_code == 201
    hold_id = r_hold.json()["id"]

    # Available balance should immediately evaluate to 5000 (lazy release of expired hold)
    r_acc = client.get(f"/api/v1/accounts/{acc_1}")
    assert r_acc.json()["available_balance"] == 5000

    # Attempting to capture expired hold must 409
    r_cap = client.post(f"/api/v1/holds/{hold_id}/capture", json={"destination_account_id": "any", "capture_amount": 1000})
    assert r_cap.status_code == 409

def test_double_entry_zero_sum_invariants():
    db = get_db()
    cursor = db.execute("SELECT SUM(amount) as net FROM ledger_entries")
    row = cursor.fetchone()
    net = row["net"] if row["net"] is not None else 0
    assert net == 0

def test_hmac_signature_and_clock_skew():
    tenant_secret = b"test_secret_key_12345"
    body = b'{"tenant_id": "t1"}'
    
    # 1. Valid Signature
    now_ts = str(int(time.time()))
    sig = hmac.new(tenant_secret, f"{now_ts}.".encode() + body, hashlib.sha256).hexdigest()
    r_valid = client.post("/api/v1/admin/reconcile", content=body, headers={
        "X-Timestamp": now_ts,
        "X-Signature-SHA256": sig,
        "Content-Type": "application/json"
    })
    assert r_valid.status_code == 200

    # 2. Skewed timestamp (> 300s) -> 401
    old_ts = str(int(time.time()) - 350)
    sig_old = hmac.new(tenant_secret, f"{old_ts}.".encode() + body, hashlib.sha256).hexdigest()
    r_skew = client.post("/api/v1/admin/reconcile", content=body, headers={
        "X-Timestamp": old_ts,
        "X-Signature-SHA256": sig_old,
        "Content-Type": "application/json"
    })
    assert r_skew.status_code == 401
