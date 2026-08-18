import pytest
from fastapi.testclient import TestClient
from main import app

client = TestClient(app)

def test_app_routes_exist():
    routes = [route.path for route in app.routes]
    assert "/api/v1/sagas" in routes
    assert "/api/v1/sagas/{saga_id}" in routes
    assert "/api/v1/sagas/{saga_id}/steps" in routes
    assert "/api/v1/sagas/{saga_id}/journal" in routes
    assert "/api/v1/sagas/{saga_id}/pause" in routes
    assert "/api/v1/sagas/{saga_id}/resume" in routes
