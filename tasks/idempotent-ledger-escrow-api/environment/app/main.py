from fastapi import FastAPI, HTTPException, Request, Header
from fastapi.responses import JSONResponse
from typing import Optional
from models import AccountCreate, TransferRequest, HoldCreate, HoldCapture
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

@app.post("/api/v1/accounts", status_code=201)
async def create_account(payload: AccountCreate):
    # TODO: Implement Account Creation with initial balanced ledger entries
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/accounts/{account_id}")
def get_account(account_id: str):
    # TODO: Implement Account balance & available balance retrieval
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/transfers")
async def transfer(req: Request, idempotency_key: Optional[str] = Header(None, alias="Idempotency-Key"), x_tenant_id: Optional[str] = Header("default", alias="X-Tenant-ID")):
    # TODO: Implement Idempotent Transfer with Double-Entry Zero-Sum Ledger records
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds", status_code=201)
async def create_hold(payload: HoldCreate):
    # TODO: Implement Escrow Hold Creation
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds/{hold_id}/capture")
async def capture_hold(hold_id: str, payload: HoldCapture):
    # TODO: Implement Two-Phase Escrow Capture (full/partial) with ledger settlement
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds/{hold_id}/void")
def void_hold(hold_id: str):
    # TODO: Implement Escrow Void
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/admin/reconcile")
async def admin_reconcile(req: Request, x_timestamp: Optional[str] = Header(None, alias="X-Timestamp"), x_signature_sha256: Optional[str] = Header(None, alias="X-Signature-SHA256")):
    # TODO: Implement HMAC-SHA256 Reconcile with Clock Skew Check
    raise HTTPException(status_code=501, detail="Not Implemented")
