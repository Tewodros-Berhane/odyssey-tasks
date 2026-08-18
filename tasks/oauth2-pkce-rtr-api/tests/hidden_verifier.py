import pytest
import hashlib
import base64
import uuid
import time
import jwt
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
from fastapi.testclient import TestClient
from main import app
from database import init_db

client = TestClient(app)

@pytest.fixture(autouse=True)
def setup():
    init_db()

def generate_pkce_pair(verifier_len=43):
    verifier = base64.urlsafe_b64encode(uuid.uuid4().bytes * 4)[:verifier_len].decode('ascii')
    digest = hashlib.sha256(verifier.encode('ascii')).digest()
    challenge = base64.urlsafe_b64encode(digest).rstrip(b'=').decode('ascii')
    return verifier, challenge

def test_authorize_and_issue():
    v, c = generate_pkce_pair()
    r = client.post("/oauth/authorize", json={"client_id": "client1", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256", "scope": "read:profile"})
    assert r.status_code == 200
    code = r.json()["authorization_code"]

    r2 = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "client1", "code": code, "code_verifier": v})
    assert r2.status_code == 200
    data = r2.json()
    assert "access_token" in data
    assert "refresh_token" in data
    assert data.get("token_type") == "Bearer"
    
    # Verify JWT is well-formed with 3 parts
    parts = data["access_token"].split(".")
    assert len(parts) == 3

def test_pkce_padding_strictness():
    v = "a" * 50
    digest = hashlib.sha256(v.encode('ascii')).digest()
    challenge = base64.urlsafe_b64encode(digest).rstrip(b'=').decode('ascii')
    
    r = client.post("/oauth/authorize", json={"client_id": "client1", "response_type": "code", "code_challenge": challenge, "code_challenge_method": "S256"})
    code = r.json()["authorization_code"]
    
    r2 = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "client1", "code": code, "code_verifier": v})
    assert r2.status_code == 200

def test_refresh_token_rotation_and_cascade():
    v, c = generate_pkce_pair()
    code = client.post("/oauth/authorize", json={"client_id": "c2", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256"}).json()["authorization_code"]
    t1 = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "c2", "code": code, "code_verifier": v}).json()
    rt1 = t1["refresh_token"]
    
    # Rotate 1
    t2 = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt1}).json()
    rt2 = t2["refresh_token"]
    assert rt1 != rt2

    # Rotate 2
    t3 = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt2}).json()
    rt3 = t3["refresh_token"]
    assert rt2 != rt3

    # ATTACK: Replay rt1 (already used)
    r_attack = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt1})
    assert r_attack.status_code == 400
    assert "invalid_grant" in r_attack.json().get("error", "")

    # VERIFY CASCADE: rt3 (the newest valid token) must now be revoked
    r_check = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt3})
    assert r_check.status_code == 400

def test_jwks_and_oidc_discovery():
    r_disc = client.get("/.well-known/openid-configuration")
    assert r_disc.status_code == 200
    disc = r_disc.json()
    assert "token_endpoint" in disc
    assert "jwks_uri" in disc

    r_jwks = client.get("/.well-known/jwks.json")
    assert r_jwks.status_code == 200
    jwks = r_jwks.json()
    assert "keys" in jwks
    assert len(jwks["keys"]) > 0
    key = jwks["keys"][0]
    assert key["kty"] == "RSA"
    assert "n" in key
    assert "e" in key

def test_token_introspection_and_revocation():
    v, c = generate_pkce_pair()
    code = client.post("/oauth/authorize", json={"client_id": "c3", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256", "scope": "admin"}).json()["authorization_code"]
    tokens = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "c3", "code": code, "code_verifier": v}).json()
    at = tokens["access_token"]

    # Introspect active
    r_intro = client.post("/oauth/introspect", json={"token": at})
    assert r_intro.status_code == 200
    assert r_intro.json().get("active") is True

    # Revoke
    r_rev = client.post("/oauth/revoke", json={"token": at})
    assert r_rev.status_code == 200

    # Introspect after revocation
    r_intro2 = client.post("/oauth/introspect", json={"token": at})
    assert r_intro2.status_code == 200
    assert r_intro2.json().get("active") is False

def test_client_credentials_and_device_flow():
    # Client credentials
    r_cc = client.post("/oauth/token", json={"grant_type": "client_credentials", "client_id": "m2m_client", "client_secret": "secret", "scope": "service"})
    assert r_cc.status_code == 200
    assert "access_token" in r_cc.json()

    # Device code authorization
    r_dev = client.post("/oauth/device/code", json={"client_id": "smart_tv", "scope": "read"})
    assert r_dev.status_code == 200
    dev = r_dev.json()
    assert "device_code" in dev
    assert "user_code" in dev
    assert "verification_uri" in dev
