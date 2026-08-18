# Enterprise FAPI 2.0 Authorization Server with JWE & DPoP

## Overview
Your objective is to complete and harden an enterprise-grade, Financial-grade API (FAPI 2.0) compliant OAuth 2.0 and OpenID Connect Authorization Framework in Python/FastAPI using SQLite and Cryptography in `/app`.

The application is structured into a modular repository:
- `core/`: Cryptographic operations (`crypto_engine.py`), PKCE verification (`pkce_validator.py`), DPoP proof verification (`dpop_engine.py`), and SQLite persistence (`database.py`).
- `models/`: Typed Pydantic models for all RFC requests and responses (`schemas.py`).
- `services/`: Specialized domain services for client registration, token management & lineage tracking, PAR orchestration, and device flow.
- `api/v1/`: Modular FastAPI route handlers.

## Core Requirements & Specifications

### 1. Dynamic Client Registration (RFC 7591)
- `POST /oauth/register` (Status `201 Created`):
  - Ingests `client_name`, `redirect_uris` (list of strings), `grant_types`, and optional `token_endpoint_auth_method`.
  - Persists the client in SQLite and returns `{"client_id": "...", "client_secret": "...", "client_name": "...", "redirect_uris": [...], "grant_types": [...]}`.

### 2. Pushed Authorization Requests (PAR, RFC 9126)
- `POST /oauth/par` (Status `201 Created`):
  - Ingests `client_id`, `response_type`, `code_challenge`, `code_challenge_method="S256"`, `redirect_uri`, and `scope`.
  - Validates client existence, records parameters, and generates a single-use `request_uri` (`urn:ietf:params:oauth:request_uri:<id>`) valid for 60 seconds.
  - Returns `{"request_uri": "...", "expires_in": 60}`.

### 3. Authorization Code & PKCE (RFC 6749, RFC 7636)
- `POST /oauth/authorize`:
  - Accepts standard authorization parameters or `request_uri` (resolving the pushed PAR payload). Single-use PAR requests must be invalidated immediately.
  - Returns `{"authorization_code": "<code>"}`.
- `POST /oauth/token` (grant_type: `authorization_code`):
  - Ingests `code`, `code_verifier`, `client_id`.
  - Optional `DPoP` HTTP header containing client's self-signed DPoP proof JWT.
  - If `DPoP` header is provided: computes the client's JWK thumbprint (SHA-256 over canonical JWK) and embeds `{"cnf": {"jkt": "<thumbprint>"}}` in the minted RS256 JWT access token. Returns `{"token_type": "DPoP", "access_token": "...", "refresh_token": "...", "expires_in": 3600}`.
  - If no DPoP header: returns standard `{"token_type": "Bearer", ...}`.

### 4. Strict Refresh Token Rotation (RTR) & Multi-Generational Invalidation
- `POST /oauth/token` (grant_type: `refresh_token`):
  - Ingests `refresh_token`.
  - **Normal Rotation:** Marks current refresh token as `used`, issues a new `access_token` and new `refresh_token` under the same `family_id`.
  - **Multi-Generational Reuse Trap:** If any previously `used` refresh token in that family is replayed, immediately mark all descendant and ancestor tokens in that `family_id` as `revoked`, and return `400 Bad Request` with top-level `{"error": "invalid_grant"}`.

### 5. DPoP Proof Verification & Protected Resource (RFC 9449)
- `POST /api/v1/protected/resource`:
  - Requires `Authorization` header (`DPoP <token>` or `Bearer <token>`) and `DPoP` header.
  - Validates DPoP proof signature using the embedded JWK, checks `htm` (HTTP method) and `htu` (request URL), checks access token hash `ath` (`base64url(sha256(access_token))`), enforces `jti` replay caching (rejecting replayed proofs with `401 Unauthorized`), and verifies that the public key matches the `cnf.jkt` claim inside the access token.
  - Returns `200 OK` with `{"data": "secure_resource", "sub": "..."}`.

### 6. OIDC Discovery & UserInfo (RFC 8414, RFC 7517, RFC 6750)
- `GET /.well-known/openid-configuration`: Returns OIDC discovery metadata.
- `GET /.well-known/jwks.json`: Returns public RSA signing keys.
- `GET /oauth/userinfo`: Validates token and returns user claims (`sub`, `name`, `email`, `email_verified`).

### 7. Introspection, Revocation, and Device Flow (RFC 7662, 7009, 8628)
- `POST /oauth/introspect`: Returns `{"active": true, ...}` or `{"active": false}`.
- `POST /oauth/revoke`: Revokes token and returns `200 OK` with `{"ok": true}`.
- `POST /oauth/device/code`: Issues `device_code`, `user_code`, `verification_uri`.
- `POST /oauth/device/verify`: Allows user to approve/deny `user_code`.
- `POST /oauth/token` (grant_type: `urn:ietf:params:oauth:grant-type:device_code`): Returns `400` with `{"error": "authorization_pending"}` until approved.

## Verification
Public smoke tests can be run via `pytest /app/tests/public_test.py`.
The sealed verifier asserts all 8 FAPI 2.0 specification domains under concurrent multi-threaded load.
