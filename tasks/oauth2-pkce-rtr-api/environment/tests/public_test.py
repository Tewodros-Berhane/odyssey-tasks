import pytest
from fastapi.testclient import TestClient
from main import app

client = TestClient(app)

def test_app_endpoints_exist():
    # Verify all expected route paths exist in the FastAPI route table
    routes = [route.path for route in app.routes]
    assert "/.well-known/openid-configuration" in routes
    assert "/.well-known/jwks.json" in routes
    assert "/oauth/authorize" in routes
    assert "/oauth/token" in routes
    assert "/oauth/introspect" in routes
    assert "/oauth/revoke" in routes
    assert "/oauth/device/code" in routes
