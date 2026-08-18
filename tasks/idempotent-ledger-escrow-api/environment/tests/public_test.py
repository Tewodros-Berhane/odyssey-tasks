import pytest
from fastapi.testclient import TestClient
from main import app
from models import AccountCreate, TransferRequest, HoldCreate, HoldCapture

client = TestClient(app)

def test_routes_exist():
    routes = [route.path for route in app.routes]
    assert "/api/v1/accounts" in routes
    assert "/api/v1/accounts/{account_id}" in routes
    assert "/api/v1/transfers" in routes
    assert "/api/v1/holds" in routes
    assert "/api/v1/holds/{hold_id}/capture" in routes
    assert "/api/v1/holds/{hold_id}/void" in routes
    assert "/api/v1/admin/reconcile" in routes

def test_pydantic_model_validations():
    acc = AccountCreate(tenant_id="t1", currency="USD", initial_balance=100)
    assert acc.initial_balance == 100

    hold = HoldCreate(account_id="acc_1", amount=50, currency="USD", expires_at="2026-08-18T12:00:00Z")
    assert hold.amount == 50
