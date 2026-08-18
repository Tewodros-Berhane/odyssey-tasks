# Enterprise FAPI OAuth 2.0 & OIDC Authorization Server

## Overview
Your objective is to implement a Financial-grade API (FAPI) compliant OAuth 2.0 and OpenID Connect Authorization Server in Python/FastAPI using SQLite and Cryptography in `/app`.

The application must support modern high-security profile specifications:
1. **RFC 7636 (PKCE)**: S256 Code Exchange.
2. **RFC 6749 & RTR**: Authorization Code, Refresh Token Rotation (RTR) with family-wide cascading invalidation upon reuse, and Client Credentials.
3. **RFC 9126 (PAR)**: Pushed Authorization Requests with single-use 60s `request_uri`.
4. **RFC 9449 (DPoP)**: Demonstrating Proof-of-Possession at the Application Layer with public key thumbprint binding (`cnf.jkt`) and `jti` replay caching.
5. **RFC 7517 & RFC 8414**: Ephemeral RSA JWKS key rotation and OpenID Connect discovery.
6. **RFC 7662 & RFC 7009**: Token Introspection and Revocation.
7. **RFC 8628**: Device Authorization Grant.

## Core API Endpoints

### 1. Discovery & JWKS (RFC 8414, RFC 7517)
- `GET /.well-known/openid-configuration`: Returns OIDC discovery metadata including `pushed_authorization_request_endpoint`, `dpop_signing_alg_values_supported: ["RS256", "ES256"]`.
- `GET /.well-known/jwks.json`: Returns public RSA signing keys with `kid`, `kty="RSA"`, `alg="RS256"`, `use="sig"`, `n`, `e`.

### 2. Pushed Authorization Requests (PAR, RFC 9126)
- `POST /oauth/par`:
  - Ingests `client_id`, `response_type`, `code_challenge`, `code_challenge_method="S256"`, `redirect_uri`, and `scope`.
  - Generates a short-lived `request_uri` (`urn:ietf:params:oauth:request_uri:<id>`) valid for 60 seconds.
  - Returns `201 Created` with `{"request_uri": "...", "expires_in": 60}`.

### 3. Authorization Code & PKCE (RFC 6749, RFC 7636)
- `POST /oauth/authorize`:
  - Accepts standard authorization parameters or `request_uri` (resolving the pushed PAR payload).
  - Returns `{"authorization_code": "<code>"}`.
- `POST /oauth/token` (grant_type: `authorization_code`):
  - Ingests `code`, `code_verifier`, `client_id`.
  - Optional `DPoP` HTTP header containing client's self-signed DPoP proof JWT.
  - If `DPoP` header is provided: computes the client's JWK thumbprint (SHA-256 over canonical JWK) and embeds `{"cnf": {"jkt": "<thumbprint>"}}` in the minted RS256 JWT access token. Returns `{"token_type": "DPoP", "access_token": "...", "refresh_token": "...", "expires_in": 3600}`.
  - If no DPoP header: returns standard `{"token_type": "Bearer", ...}`.

### 4. Strict Refresh Token Rotation (RTR) & Cascading Revocation
- `POST /oauth/token` (grant_type: `refresh_token`):
  - Ingests `refresh_token`.
  - **Normal Rotation:** Marks token as `used`, issues new `access_token` and new `refresh_token` under the same `family_id`.
  - **Reuse Trap:** If token is already `used`, immediately sets all tokens in that `family_id` to `revoked` and returns `400 Bad Request` with top-level `{"error": "invalid_grant"}`.

### 5. DPoP Proof Verification & Resource Access (RFC 9449)
- `POST /api/v1/protected/resource`:
  - Requires `Authorization` header (`DPoP <token>` or `Bearer <token>`) and `DPoP` header.
  - Validates:
    1. DPoP proof signature using public key in DPoP JWT `jwk` header.
    2. `htm` matches HTTP method (`POST`) and `htu` matches request URL.
    3. `jti` replay protection: Replayed `jti` returns `401 Unauthorized` with `{"error": "invalid_dpop_proof"}`.
    4. Client's JWK thumbprint matches `cnf.jkt` claim inside access token.
  - Returns `200 OK` with `{"data": "secure_resource", "sub": "..."}`.

### 6. Introspection, Revocation, and Device Flow
- `POST /oauth/introspect` (RFC 7662): Returns `{"active": true, ...}` or `{"active": false}`.
- `POST /oauth/revoke` (RFC 7009): Revokes token and returns `200 OK` with `{"ok": true}`.
- `POST /oauth/device/code` (RFC 8628): Issues `device_code`, `user_code`, `verification_uri`, `interval`.

## Verification
Public smoke tests can be run via `pytest /app/tests/public_test.py`.
The sealed verifier asserts all 7 FAPI specification domains, DPoP proof-of-possession binding, and token family replay defenses.
