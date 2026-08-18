# Production OAuth 2.0 & OIDC Authorization Server with PKCE, RTR, and JWKS

## Overview
Your objective is to implement a comprehensive, RFC-compliant OAuth 2.0 and OpenID Connect Authorization Server in Python/FastAPI using SQLite and Cryptography in `/app`.

## Core Protocols & Specifications

### 1. OpenID Connect Discovery & JWKS (RFC 8414, RFC 7517)
- `GET /.well-known/openid-configuration`:
  - Returns JSON containing `issuer`, `authorization_endpoint`, `token_endpoint`, `jwks_uri`, `response_types_supported`, `grant_types_supported`, `id_token_signing_alg_values_supported: ["RS256"]`.
- `GET /.well-known/jwks.json`:
  - Generates an RSA keypair on startup and publishes the public key as an RFC 7517 JWK containing `kty="RSA"`, `kid`, `use="sig"`, `alg="RS256"`, `n`, `e` (where `n` and `e` are unpadded Base64URL-encoded big-endian integer bytes).

### 2. PKCE Authorization Code Grant (RFC 7636 & RFC 6749)
- `POST /oauth/authorize`:
  - Ingests `client_id`, `response_type="code"`, `code_challenge`, `code_challenge_method="S256"`, and `scope`.
  - Returns `{"authorization_code": "<code>"}`.
- `POST /oauth/token` (grant_type: `authorization_code`):
  - Ingests `code`, `code_verifier`, and `client_id`.
  - Hashes `code_verifier` with SHA-256 and validates against the stored unpadded Base64URL `code_challenge`.
  - Returns `200 OK` with `{"access_token": "<signed RS256 JWT>", "token_type": "Bearer", "refresh_token": "<opaque_rt>", "expires_in": 3600, "scope": "<scope>"}`.

### 3. Strict Refresh Token Rotation (RTR) & Token Family Cascade (RFC 6749)
- `POST /oauth/token` (grant_type: `refresh_token`):
  - Ingests `refresh_token`.
  - **Normal Rotation:** If active, marks the current refresh token as `used`, issues a new RS256 signed JWT `access_token` and new `refresh_token` under the same `family_id`.
  - **Replay Attack Trap:** If the presented refresh token has already been marked as `used`, immediately revoke the **entire token family** (setting status of all tokens in that family to `revoked`) and return `400 Bad Request` with top-level `{"error": "invalid_grant"}`.

### 4. Token Introspection & Revocation (RFC 7662 & RFC 7009)
- `POST /oauth/introspect`:
  - Ingests `token`. If token is active and not revoked, returns `{"active": true, "scope": "...", "sub": "...", "exp": ...}`. If revoked or unknown, returns `{"active": false}`.
- `POST /oauth/revoke`:
  - Ingests `token`. Marks the token status as `revoked` and returns `200 OK` with `{"ok": true}`.

### 5. Client Credentials & Device Authorization Flow (RFC 6749 & RFC 8628)
- `POST /oauth/token` (grant_type: `client_credentials`):
  - Ingests `client_id`, `client_secret`, and `scope`. Returns a signed RS256 JWT `access_token`.
- `POST /oauth/device/code`:
  - Ingests `client_id` and `scope`. Issues `{"device_code": "...", "user_code": "...", "verification_uri": "...", "expires_in": 600, "interval": 5}`.

## Verification
You can run the public smoke tests via `pytest /app/tests/public_test.py`.
The sealed verifier validates all grant flows, cryptographic RS256 JWT signatures against JWKS, and cascading replay detection attacks.
