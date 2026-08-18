from pydantic import BaseModel
from typing import Optional

class AccountCreate(BaseModel):
    id: Optional[str] = None
    tenant_id: str
    currency: str
    initial_balance: Optional[int] = 0

class TransferRequest(BaseModel):
    from_account_id: str
    to_account_id: str
    amount: int

class HoldCreate(BaseModel):
    account_id: str
    amount: int
    currency: str
    expires_at: str

class HoldCapture(BaseModel):
    destination_account_id: str
    capture_amount: Optional[int] = None
