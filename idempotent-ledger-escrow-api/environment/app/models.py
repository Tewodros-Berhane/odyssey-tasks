from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime

class AccountCreate(BaseModel):
    id: Optional[str] = Field(None, description="Optional custom ID. Generated if missing.")
    tenant_id: str = Field(..., description="The ID of the tenant owning the account")
    currency: str = Field(..., min_length=3, max_length=3, description="ISO 4217 currency code")
    initial_balance: int = Field(default=0, ge=0, description="Initial balance in minor units (cents)")

class TransferRequest(BaseModel):
    from_account_id: str
    to_account_id: str
    amount: int = Field(..., gt=0, description="Amount to transfer in minor units")

class HoldCreate(BaseModel):
    account_id: str
    amount: int = Field(..., gt=0, description="Amount to hold in minor units")
    currency: str = Field(..., min_length=3, max_length=3)
    expires_at: datetime = Field(..., description="ISO-8601 UTC timestamp for hold expiration")

class HoldCapture(BaseModel):
    destination_account_id: str
    capture_amount: Optional[int] = Field(None, gt=0, description="Amount to capture. Defaults to full held amount if omitted.")