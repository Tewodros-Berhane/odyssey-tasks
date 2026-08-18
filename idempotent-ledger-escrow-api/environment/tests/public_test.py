import pytest
from fastapi.testclient import TestClient
from main import app
from database import init_db

client = TestClient(app)

@pytest.fixture(autouse=True)
def setup_db():
    init_db()

def test_account_creation_smoke():
    res = client.post("/api/v1/accounts", json={"id": "acc_001", "tenant_id": "t1", "currency": "USD", "initial_balance": 1000})
    assert res.status_code in [200, 201]
    data = res.json()
    assert data["balance"] == 1000
    assert data["available_balance"] == 1000