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
        CREATE TABLE IF NOT EXISTS auth_codes (
            code TEXT PRIMARY KEY,
            client_id TEXT NOT NULL,
            challenge TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            used INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            family_id TEXT NOT NULL,
            token_type TEXT NOT NULL, -- 'access_token', 'refresh_token'
            scope TEXT DEFAULT 'openid',
            status TEXT NOT NULL      -- 'active', 'used', 'revoked'
        );
        CREATE TABLE IF NOT EXISTS device_codes (
            device_code TEXT PRIMARY KEY,
            user_code TEXT NOT NULL,
            client_id TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            status TEXT NOT NULL DEFAULT 'pending'
        );
        """)
EOF

cat << 'EOF' > /app/main.py
from fastapi import FastAPI
from fastapi.responses import JSONResponse
from models import AuthorizeRequest, TokenRequest, IntrospectRequest, RevokeRequest, DeviceCodeRequest
from database import init_db, get_db
import uuid
import hashlib
import base64

app = FastAPI()

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
        "response_types_supported": ["code"],
        "grant_types_supported": ["authorization_code", "refresh_token", "client_credentials", "urn:ietf:params:oauth:grant-type:device_code"]
    }

@app.get("/.well-known/jwks.json")
async def jwks():
    return {
        "keys": [
            {
                "kty": "RSA",
                "kid": "key-2026-01",
                "use": "sig",
                "alg": "RS256",
                "n": "u1W1x...",
                "e": "AQAB"
            }
        ]
    }

@app.post("/oauth/authorize")
async def authorize(payload: AuthorizeRequest):
    code = f"auth_{uuid.uuid4().hex}"
    conn = get_db()
    with conn:
        conn.execute("INSERT INTO auth_codes (code, client_id, challenge, scope) VALUES (?, ?, ?, ?)", 
                     (code, payload.client_id, payload.code_challenge, payload.scope or "openid"))
    return {"authorization_code": code}

@app.post("/oauth/token")
async def token(payload: TokenRequest):
    conn = get_db()
    if payload.grant_type == "authorization_code":
        code = payload.code
        verifier = payload.code_verifier or ""
        with conn:
            row = conn.execute("SELECT * FROM auth_codes WHERE code = ? AND client_id = ? AND used = 0", (code, payload.client_id)).fetchone()
            if not row:
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
            
            digest = hashlib.sha256(verifier.encode('ascii')).digest()
            challenge = base64.urlsafe_b64encode(digest).rstrip(b'=').decode('ascii')
            
            if challenge != row["challenge"]:
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            conn.execute("UPDATE auth_codes SET used = 1 WHERE code = ?", (code,))
            family_id = f"fam_{uuid.uuid4().hex}"
            at = f"at_{uuid.uuid4().hex}"
            rt = f"rt_{uuid.uuid4().hex}"
            scope = row["scope"]
            
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, status) VALUES (?, ?, 'access_token', ?, 'active')", (at, family_id, scope))
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, status) VALUES (?, ?, 'refresh_token', ?, 'active')", (rt, family_id, scope))
            
            return {"access_token": at, "token_type": "Bearer", "refresh_token": rt, "expires_in": 3600, "scope": scope}

    elif payload.grant_type == "refresh_token":
        rt = payload.refresh_token
        with conn:
            row = conn.execute("SELECT * FROM tokens WHERE token = ?", (rt,)).fetchone()
            if not row or row["status"] == "revoked":
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            if row["status"] == "used":
                # REUSE DETECTED: Revoke entire family
                conn.execute("UPDATE tokens SET status = 'revoked' WHERE family_id = ?", (row["family_id"],))
                return JSONResponse(status_code=400, content={"error": "invalid_grant"})
                
            # Valid rotation
            conn.execute("UPDATE tokens SET status = 'used' WHERE token = ?", (rt,))
            new_at = f"at_{uuid.uuid4().hex}"
            new_rt = f"rt_{uuid.uuid4().hex}"
            scope = row["scope"]
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, status) VALUES (?, ?, 'access_token', ?, 'active')", (new_at, row["family_id"], scope))
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, status) VALUES (?, ?, 'refresh_token', ?, 'active')", (new_rt, row["family_id"], scope))
            
            return {"access_token": new_at, "token_type": "Bearer", "refresh_token": new_rt, "expires_in": 3600, "scope": scope}
            
    elif payload.grant_type == "client_credentials":
        at = f"m2m_at_{uuid.uuid4().hex}"
        family_id = f"m2m_fam_{uuid.uuid4().hex}"
        scope = payload.scope or "service"
        with conn:
            conn.execute("INSERT INTO tokens (token, family_id, token_type, scope, status) VALUES (?, ?, 'access_token', ?, 'active')", (at, family_id, scope))
        return {"access_token": at, "token_type": "Bearer", "expires_in": 7200, "scope": scope}
    
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
        "exp": 1893456000
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
EOF
