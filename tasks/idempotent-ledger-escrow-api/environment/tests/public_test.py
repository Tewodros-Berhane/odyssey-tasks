import pytest
from fastapi.testclient import TestClient
from main import app

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
