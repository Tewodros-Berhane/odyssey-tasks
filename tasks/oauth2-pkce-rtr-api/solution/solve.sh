#!/usr/bin/env bash
set -e

cat << 'EOF' > /app/database.py
import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "/app/oauth.db")

def get_db():
    conn = sqlite3.connect(DB_PATH, timeout=10.0, isolation_level=None)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    with conn:
        conn.executescript("""
        CREATE TABLE IF NOT EXISTS par_requests (
            request_uri TEXT PRIMARY KEY,
            client_id TEXT NOT NULL,
            challenge TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            expires_at TIMESTAMP NOT NULL
        );
        CREATE TABLE IF NOT EXISTS auth_codes (
            code TEXT PRIMARY KEY,
            client_id TEXT NOT NULL,
            challenge TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            dpop_jkt TEXT,
            used INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            family_id TEXT NOT NULL,
            token_type TEXT NOT NULL, -- 'access_token', 'refresh_token'
            scope TEXT DEFAULT 'openid',
            dpop_jkt TEXT,
            status TEXT NOT NULL      -- 'active', 'used', 'revoked'
        );
        CREATE TABLE IF NOT EXISTS device_codes (
            device_code TEXT PRIMARY KEY,
            user_code TEXT NOT NULL,
            client_id TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            status TEXT NOT NULL DEFAULT 'pending'
        );
        CREATE TABLE IF NOT EXISTS dpop_jti_cache (
            jti TEXT PRIMARY KEY,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        """)
EOF

cat << 'EOF' > /app/main.py
import uuid
import hashlib
import base64
import time
import json
import jwt
from datetime import datetime, timezone
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization
from fastapi import FastAPI, Request, Header
from fastapi.responses import JSONResponse
from models import ParRequest, AuthorizeRequest, TokenRequest, IntrospectRequest, RevokeRequest, DeviceCodeRequest
from database import init_db, get_db

app = FastAPI()

# Server RSA Keypair
server_priv = rsa.generate_private_key(public_exponent=65537, key_size=2048)
server_pub = server_priv.public_key()
server_pub_num = server_pub.public_numbers()

def int_to_b64(val: int) -> str:
    val_bytes = val.to_bytes((val.bit_length() + 7) // 8, byteorder="big")
    return base64.urlsafe_b64encode(val_bytes).rstrip(b"=").decode("ascii")

server_jwk_n = int_to_b64(server_pub_num.n)
server_jwk_e = int_to_b64(server_pub_num.e)
SERVER_KID = "fapi-auth-key-2026"

server_pem_priv = server_priv.private_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PrivateFormat.PKCS8,
    encryption_algorithm=serialization.NoEncryption()
).decode("ascii")

server_pem_pub = server_pub.public_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PublicFormat.SubjectPublicKeyInfo
).decode("ascii")

def compute_jwk_thumbprint(jwk_dict: dict) -> str:
    canonical = json.dumps({"e": jwk_dict["e"], "kty": jwk_dict["kty"], "n": jwk_dict["n"]}, separators=(",", ":"), sort_keys=True)
    return base64.urlsafe_b64encode(hashlib.sha256(canonical.encode("ascii")).digest()).rstrip(b"=").decode("ascii")

def mint_jwt(sub: str, scope: str, dpop_jkt: str = None, expires_in: int = 3600) -> str:
    now = int(time.time())
    payload = {
        "iss": "https://auth.example.com",
        "sub": sub,
        "aud": "https://api.example.com",
        "iat": now,
        "exp": now + expires_in,
        "scope": scope,
        "jti": uuid.uuid4().hex
    }
    if dpop_jkt:
        payload["cnf"] = {"jkt": dpop_jkt}
    return jwt.encode(payload, server_pem_priv, algorithm="RS256", headers={"kid": SERVER_KID})

@app.on_event("startup")
def startup():
    init_db()

@app.get("/.well-known/openid-configuration")
async def openid_config():
    return {
        "issuer": "https://auth.example.com",
        "authorization_endpoint": "https://auth.example.com/oauth/authorize",
        "token_endpoint": "https://auth.example.com/oauth/token",
        "jwks_uri": "https://auth.example.com/.well-known/jwks.json",
        "pushed_authorization_request_endpoint": "https://auth.example.com/oauth/par",
        "dpop_signing_alg_values_supported": ["RS256"],
        "response_types_supported": ["code"],
        "grant_types_supported": ["authorization_code", "refresh_token", "client_credentials", "urn:ietf:params:oauth:grant-type:device_code"],
        "id_token_signing_alg_values_supported": ["RS256"]
    }

@app.get("/.well-known/jwks.json")
async def jwks():
    return {
        "keys": [
            {
                "kty": "RSA",
                "kid": SERVER_KID,
                "use": "sig",
                "alg": "RS256",
                "n": server_jwk_n,
                "e": server_jwk_e
            }
        ]
    }

@app.post("/oauth/par", status_code=201)
async def par(payload: ParRequest):
    req_uri = f"urn:ietf:params:oauth:request_uri:{uuid.uuid4().hex}"
    conn = get_db()
    with conn:
        conn.execute("INSERT INTO par_requests (request_uri, client_id, challenge, scope, expires_at) VALUES (?, ?, ?, ?, datetime('now', '+60 seconds'))",
                     (req_uri, payload.client_id, payload.code_challenge, payload.scope or "openid"))
    return {"request_uri": req_uri, "expires_in": 60}

@app.post("/oauth/authorize")
async def authorize(payload: AuthorizeRequest):
    conn = get_db()
    if payload.request_uri:
        with conn:
            row = conn.execute("SELECT * FROM par_requests WHERE request_uri = ? AND expires_at > datetime('now')", (payload.request_uri,)).fetchone()
            if not row:
                return JSONResponse(status_code=400, content={"error": "invalid_request_uri"})
            code = f"auth_{uuid.uuid4().hex}"
            conn.execute("INSERT INTO auth_codes (code, client_id, challenge, scope) VALUES (?, ?, ?, ?)",
                         (code, row["client_id"], row["challenge"], row["scope"]))
            conn.execute("DELETE FROM par_requests WHERE request_uri = ?", (payload.request_uri,))
            return {"authorization_code": code}

    code = f"auth_{uuid.uuid4().hex}"
    with conn:
        conn.execute("INSERT INTO auth_codes (code, client_id, challenge, scope) VALUES (?, ?, ?, ?)", 
                     (code, payload.client_id, payload.code_challenge, payload.scope or "openid"))
    return {"authorization_code": code}

@app.post("/oauth/token")
async def token(payload: TokenRequest, req: Request):
    conn = get_db()
    dpop_header = req.headers.get("DPoP")
    dpop_jkt = None

    if dpop_header:
        try:
            unverified_header = jwt.get_unverified_header(dpop_header)
            jwk = unverified_header["jwk"]
            dpop_jkt = compute_jwk_thumbprint(jwk)
        except Exception:
            return JSONResponse(status_code=400, content={"error": "invalid_dpop_proof"})

    if payload.grant_type == "authorization_code":
        code = payload.code
        verifier = payload.code_verifier or ""
        with conn:
            row = conn.execute("SELECT * FROM auth_codes WHERE code = ? AND client_id = ? AND used = 0", (code, payload.client_id)).fetchone()
            if not row:
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
            
            digest = hashlib.sha256(verifier.encode("ascii")).digest()
            challenge = base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")
            
            if challenge != row["challenge"]:
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            conn.execute("UPDATE auth_codes SET used = 1 WHERE code = ?", (code,))
            family_id = f"fam_{uuid.uuid4().hex}"
            scope = row["scope"]
            at = mint_jwt(sub=payload.client_id, scope=scope, dpop_jkt=dpop_jkt, expires_in=3600)
            rt = f"rt_{uuid.uuid4().hex}"
            tok_type = "DPoP" if dpop_jkt else "Bearer"
            
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, dpop_jkt, status) VALUES (?, ?, 'access_token', ?, ?, 'active')", (at, family_id, scope, dpop_jkt))
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, dpop_jkt, status) VALUES (?, ?, 'refresh_token', ?, ?, 'active')", (rt, family_id, scope, dpop_jkt))
            
            return {"access_token": at, "token_type": tok_type, "refresh_token": rt, "expires_in": 3600, "scope": scope}

    elif payload.grant_type == "refresh_token":
        rt = payload.refresh_token
        with conn:
            row = conn.execute("SELECT * FROM tokens WHERE token = ?", (rt,)).fetchone()
            if not row or row["status"] == "revoked":
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            if row["status"] == "used":
                conn.execute("UPDATE tokens SET status = 'revoked' WHERE family_id = ?", (row["family_id"],))
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            conn.execute("UPDATE tokens SET status = 'used' WHERE token = ?", (rt,))
            scope = row["scope"]
            new_at = mint_jwt(sub="user", scope=scope, dpop_jkt=dpop_jkt, expires_in=3600)
            new_rt = f"rt_{uuid.uuid4().hex}"
            tok_type = "DPoP" if dpop_jkt else "Bearer"
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, dpop_jkt, status) VALUES (?, ?, 'access_token', ?, ?, 'active')", (new_at, row["family_id"], scope, dpop_jkt))
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, dpop_jkt, status) VALUES (?, ?, 'refresh_token', ?, ?, 'active')", (new_rt, row["family_id"], scope, dpop_jkt))
            
            return {"access_token": new_at, "token_type": tok_type, "refresh_token": new_rt, "expires_in": 3600, "scope": scope}
            
    elif payload.grant_type == "client_credentials":
        scope = payload.scope or "service"
        at = mint_jwt(sub=payload.client_id or "m2m_client", scope=scope, dpop_jkt=dpop_jkt, expires_in=7200)
        family_id = f"m2m_fam_{uuid.uuid4().hex}"
        tok_type = "DPoP" if dpop_jkt else "Bearer"
        with conn:
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, dpop_jkt, status) VALUES (?, ?, 'access_token', ?, ?, 'active')", (at, family_id, scope, dpop_jkt))
        return {"access_token": at, "token_type": tok_type, "expires_in": 7200, "scope": scope}
    
    return JSONResponse(status_code=400, content={"error": "unsupported_grant_type"})

@app.post("/oauth/introspect")
async def introspect(payload: IntrospectRequest):
    conn = get_db()
    row = conn.execute("SELECT * FROM tokens WHERE token = ?", (payload.token,)).fetchone()
    if not row or row["status"] != "active":
        return {"active": False}
    return {
        "active": True,
        "scope": row["scope"],
        "token_type": row["token_type"],
        "exp": int(time.time()) + 3600
    }

@app.post("/oauth/revoke")
async def revoke(payload: RevokeRequest):
    conn = get_db()
    with conn:
        conn.execute("UPDATE tokens SET status = 'revoked' WHERE token = ?", (payload.token,))
    return {"ok": True}

@app.post("/oauth/device/code")
async def device_code(payload: DeviceCodeRequest):
    dev_code = f"dev_{uuid.uuid4().hex}"
    user_code = f"WDJB-{uuid.uuid4().hex[:4].upper()}"
    conn = get_db()
    with conn:
        conn.execute("INSERT INTO device_codes (device_code, user_code, client_id, scope) VALUES (?, ?, ?, ?)",
                     (dev_code, user_code, payload.client_id, payload.scope or "openid"))
    return {
        "device_code": dev_code,
        "user_code": user_code,
        "verification_uri": "https://auth.example.com/device",
        "expires_in": 600,
        "interval": 5
    }

@app.post("/api/v1/protected/resource")
async def protected_resource(req: Request):
    auth = req.headers.get("Authorization", "")
    dpop = req.headers.get("DPoP")
    if not auth or not dpop:
        return JSONResponse(status_code=401, content={"error": "invalid_token"})

    token = auth.split(" ")[-1]
    conn = get_db()

    # 1. Parse DPoP Header & verify signature + jti replay
    try:
        unverified_header = jwt.get_unverified_header(dpop)
        jwk = unverified_header["jwk"]
        jwk_n_bytes = base64.urlsafe_b64decode(jwk["n"] + "==")
        jwk_e_bytes = base64.urlsafe_b64decode(jwk["e"] + "==")
        pub_key = rsa.RSAPublicNumbers(
            e=int.from_bytes(jwk_e_bytes, "big"),
            n=int.from_bytes(jwk_n_bytes, "big")
        ).public_key()

        dpop_claims = jwt.decode(dpop, pub_key, algorithms=["RS256"], options={"verify_exp": False})
        jti = dpop_claims["jti"]

        with conn:
            # Check JTI replay
            if conn.execute("SELECT 1 FROM dpop_jti_cache WHERE jti = ?", (jti,)).fetchone():
                return JSONResponse(status_code=401, content={"error": "invalid_dpop_proof"})
            conn.execute("INSERT INTO dpop_jti_cache (jti) VALUES (?)", (jti,))

        # Verify access token signature & cnf.jkt binding
        at_claims = jwt.decode(token, server_pem_pub, algorithms=["RS256"], options={"verify_aud": False})
        expected_jkt = compute_jwk_thumbprint(jwk)
        if at_claims.get("cnf", {}).get("jkt") != expected_jkt:
            return JSONResponse(status_code=401, content={"error": "invalid_dpop_proof"})

        return {"data": "secure_resource", "sub": at_claims.get("sub")}
    except Exception:
        return JSONResponse(status_code=401, content={"error": "invalid_dpop_proof"})
EOF
