import pytest
import hashlib
import base64
import uuid
import time
import json
import jwt
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
from fastapi.testclient import TestClient
from main import app
from database import init_db

client = TestClient(app)

# Helper: client RSA keypair for generating DPoP proofs
client_priv = rsa.generate_private_key(public_exponent=65537, key_size=2048)
client_pub = client_priv.public_key()
client_pub_num = client_pub.public_numbers()

def int_to_b64(val: int) -> str:
    val_bytes = val.to_bytes((val.bit_length() + 7) // 8, byteorder="big")
    return base64.urlsafe_b64encode(val_bytes).rstrip(b"=").decode("ascii")

client_jwk = {
    "kty": "RSA",
    "n": int_to_b64(client_pub_num.n),
    "e": int_to_b64(client_pub_num.e)
}

client_pem_priv = client_priv.private_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PrivateFormat.PKCS8,
    encryption_algorithm=serialization.NoEncryption()
).decode("ascii")

def create_dpop_proof(htm: str, htu: str, ath: str = None, jti: str = None) -> str:
    now = int(time.time())
    payload = {
        "jti": jti or uuid.uuid4().hex,
        "htm": htm,
        "htu": htu,
        "iat": now
    }
    if ath:
        payload["ath"] = ath
    headers = {
        "typ": "dpop+jwt",
        "alg": "RS256",
        "jwk": client_jwk
    }
    return jwt.encode(payload, client_pem_priv, algorithm="RS256", headers=headers)

@pytest.fixture(autouse=True)
def setup():
    init_db()

def generate_pkce_pair(verifier_len=43):
    verifier = base64.urlsafe_b64encode(uuid.uuid4().bytes * 4)[:verifier_len].decode("ascii")
    digest = hashlib.sha256(verifier.encode("ascii")).digest()
    challenge = base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")
    return verifier, challenge

def test_dynamic_client_registration_rfc7591():
    r_reg = client.post("/oauth/register", json={
        "client_name": "FAPI Client Suite",
        "redirect_uris": ["https://myclient.com/callback"],
        "grant_types": ["authorization_code", "refresh_token"]
    })
    assert r_reg.status_code == 201
    data = r_reg.json()
    assert "client_id" in data
    assert "client_secret" in data
    assert data["client_name"] == "FAPI Client Suite"
    assert data["redirect_uris"] == ["https://myclient.com/callback"]

def test_pushed_authorization_requests_par():
    # 1. Register client
    r_reg = client.post("/oauth/register", json={"client_name": "PAR App", "redirect_uris": ["https://app.com/cb"]})
    c_id = r_reg.json()["client_id"]

    v, c = generate_pkce_pair()
    # 2. Push authorization request
    r_par = client.post("/oauth/par", json={
        "client_id": c_id,
        "response_type": "code",
        "code_challenge": c,
        "code_challenge_method": "S256",
        "scope": "openid profile"
    })
    assert r_par.status_code == 201
    par_data = r_par.json()
    assert "request_uri" in par_data
    assert par_data.get("expires_in") == 60
    req_uri = par_data["request_uri"]

    # 3. Authorize using request_uri
    r_auth = client.post("/oauth/authorize", json={"request_uri": req_uri})
    assert r_auth.status_code == 200
    code = r_auth.json()["authorization_code"]

    # 4. Exchange code for token
    r_tok = client.post("/oauth/token", json={
        "grant_type": "authorization_code",
        "client_id": c_id,
        "code": code,
        "code_verifier": v
    })
    assert r_tok.status_code == 200
    assert "access_token" in r_tok.json()

def test_pkce_authorization_code_and_padding_strictness():
    v = "a" * 50
    digest = hashlib.sha256(v.encode("ascii")).digest()
    challenge = base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")

    r = client.post("/oauth/authorize", json={"client_id": "client_pkce", "response_type": "code", "code_challenge": challenge, "code_challenge_method": "S256"})
    assert r.status_code == 200
    code = r.json()["authorization_code"]

    r2 = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "client_pkce", "code": code, "code_verifier": v})
    assert r2.status_code == 200
    assert len(r2.json()["access_token"].split(".")) == 3

def test_refresh_token_rotation_and_cascade():
    v, c = generate_pkce_pair()
    code = client.post("/oauth/authorize", json={"client_id": "c2", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256"}).json()["authorization_code"]
    t1 = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "c2", "code": code, "code_verifier": v}).json()
    rt1 = t1["refresh_token"]

    t2 = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt1}).json()
    rt2 = t2["refresh_token"]
    assert rt1 != rt2

    t3 = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt2}).json()
    rt3 = t3["refresh_token"]
    assert rt2 != rt3

    # ATTACK: Replay rt1
    r_attack = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt1})
    assert r_attack.status_code == 400
    assert "invalid_grant" in r_attack.json().get("error", "")

    # Cascade check: rt3 revoked
    r_check = client.post("/oauth/token", json={"grant_type": "refresh_token", "client_id": "c2", "refresh_token": rt3})
    assert r_check.status_code == 400

def test_dpop_proof_of_possession_and_replay():
    v, c = generate_pkce_pair()
    code = client.post("/oauth/authorize", json={"client_id": "dpop_client", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256"}).json()["authorization_code"]
    
    # 1. Issue DPoP-bound token
    dpop_tok_proof = create_dpop_proof("POST", "http://testserver/oauth/token")
    r_tok = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "dpop_client", "code": code, "code_verifier": v}, headers={"DPoP": dpop_tok_proof})
    assert r_tok.status_code == 200
    assert r_tok.json().get("token_type") == "DPoP"
    at = r_tok.json()["access_token"]

    # 2. Access protected resource with valid DPoP proof
    res_url = "http://testserver/api/v1/protected/resource"
    dpop_res_proof = create_dpop_proof("POST", res_url)
    r_res = client.post("/api/v1/protected/resource", headers={"Authorization": f"DPoP {at}", "DPoP": dpop_res_proof})
    assert r_res.status_code == 200
    assert r_res.json().get("data") == "secure_resource"

    # 3. REPLAY ATTACK: Reusing same DPoP proof (same jti) must return 401
    r_replay = client.post("/api/v1/protected/resource", headers={"Authorization": f"DPoP {at}", "DPoP": dpop_res_proof})
    assert r_replay.status_code == 401

def test_userinfo_and_jwks_discovery():
    r_disc = client.get("/.well-known/openid-configuration")
    assert r_disc.status_code == 200
    disc = r_disc.json()
    assert "token_endpoint" in disc
    assert "jwks_uri" in disc
    assert "registration_endpoint" in disc
    assert "userinfo_endpoint" in disc

    r_jwks = client.get("/.well-known/jwks.json")
    assert r_jwks.status_code == 200
    assert len(r_jwks.json()["keys"]) > 0

    # Test UserInfo endpoint
    r_cc = client.post("/oauth/token", json={"grant_type": "client_credentials", "client_id": "user_123", "scope": "openid email"})
    at = r_cc.json()["access_token"]
    r_uinfo = client.get("/oauth/userinfo", headers={"Authorization": f"Bearer {at}"})
    assert r_uinfo.status_code == 200
    u_data = r_uinfo.json()
    assert u_data["email"] == "dev@odyssey.com"

def test_token_introspection_and_revocation():
    v, c = generate_pkce_pair()
    code = client.post("/oauth/authorize", json={"client_id": "c3", "response_type": "code", "code_challenge": c, "code_challenge_method": "S256"}).json()["authorization_code"]
    tokens = client.post("/oauth/token", json={"grant_type": "authorization_code", "client_id": "c3", "code": code, "code_verifier": v}).json()
    at = tokens["access_token"]

    r_intro = client.post("/oauth/introspect", json={"token": at})
    assert r_intro.status_code == 200
    assert r_intro.json().get("active") is True

    r_rev = client.post("/oauth/revoke", json={"token": at})
    assert r_rev.status_code == 200

    r_intro2 = client.post("/oauth/introspect", json={"token": at})
    assert r_intro2.status_code == 200
    assert r_intro2.json().get("active") is False

def test_device_flow_and_user_verification():
    r_dev = client.post("/oauth/device/code", json={"client_id": "tv_client"})
    assert r_dev.status_code == 200
    dev_data = r_dev.json()
    dev_code = dev_data["device_code"]
    u_code = dev_data["user_code"]

    # Before user approval, polling returns 400 authorization_pending
    r_poll1 = client.post("/oauth/token", json={"grant_type": "urn:ietf:params:oauth:grant-type:device_code", "client_id": "tv_client", "device_code": dev_code})
    assert r_poll1.status_code == 400
    assert r_poll1.json().get("error") == "authorization_pending"

    # User verifies code
    r_verify = client.post("/oauth/device/verify", json={"user_code": u_code, "approved": True})
    assert r_verify.status_code == 200

    # After approval, polling returns access token
    r_poll2 = client.post("/oauth/token", json={"grant_type": "urn:ietf:params:oauth:grant-type:device_code", "client_id": "tv_client", "device_code": dev_code})
    assert r_poll2.status_code == 200
    assert "access_token" in r_poll2.json()
