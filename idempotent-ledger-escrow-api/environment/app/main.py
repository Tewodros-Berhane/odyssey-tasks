from fastapi import FastAPI, Header, HTTPException
from typing import Optional
from models import AccountCreate, TransferRequest, HoldCreate, HoldCapture
from database import init_db

app = FastAPI(title="Idempotent Escrow Ledger API")

@app.on_event("startup")
def startup():
    init_db()

@app.post("/api/v1/accounts")
async def create_account(payload: AccountCreate):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/accounts/{account_id}")
async def get_account(account_id: str):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/transfers")
async def transfer_funds(
    payload: TransferRequest, 
    idempotency_key: Optional[str] = Header(None, alias="Idempotency-Key"),
    x_tenant_id: Optional[str] = Header("default", alias="X-Tenant-ID")
):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds")
async def create_hold(payload: HoldCreate):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds/{hold_id}/capture")
async def capture_hold(hold_id: str, payload: HoldCapture):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/holds/{hold_id}/void")
async def void_hold(hold_id: str):
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/admin/reconcile")
async def admin_reconcile():
    raise HTTPException(status_code=501, detail="Not Implemented")