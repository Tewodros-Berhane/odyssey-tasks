import pytest
from fastapi.testclient import TestClient
from main import app
from models import ClientRegisterRequest, ParRequest, AuthorizeRequest, TokenRequest

client = TestClient(app)

def test_routes_exist():
    routes = [route.path for route in app.routes]
    assert "/.well-known/openid-configuration" in routes
    assert "/.well-known/jwks.json" in routes
    assert "/oauth/register" in routes
    assert "/oauth/par" in routes
    assert "/oauth/authorize" in routes
    assert "/oauth/token" in routes
    assert "/oauth/userinfo" in routes
    assert "/oauth/introspect" in routes
    assert "/oauth/revoke" in routes
    assert "/oauth/device/code" in routes
    assert "/oauth/device/verify" in routes
    assert "/api/v1/protected/resource" in routes

def test_model_validations():
    reg = ClientRegisterRequest(client_name="Test App", redirect_uris=["https://example.com/callback"])
    assert reg.client_name == "Test App"
    assert "https://example.com/callback" in reg.redirect_uris
