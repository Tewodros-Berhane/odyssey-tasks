# Problem Statement: Production OAuth 2.0 & OIDC Authorization Server

You must implement a full-featured, RFC-compliant OAuth 2.0 and OpenID Connect Authorization Server in Python/FastAPI using SQLite in `/app`.

## Core Requirements

### 1. PKCE Authorization Code Grant (RFC 7636 & RFC 6749)
- `POST /oauth/authorize`:
  - Receives `client_id`, `response_type=code`, `code_challenge`, `code_challenge_method=S256`, and optional `scope`.
  - Returns `{"authorization_code": "<code>"}`.
- `POST /oauth/token` (grant_type: `authorization_code`):
  - Receives `code`, `code_verifier`, and `client_id`.
  - Hashes `code_verifier` using SHA-256 and validates against unpadded Base64URL `code_challenge`.
  - Mints an RS256/HS256 signed JWT `access_token` and an opaque `refresh_token` belonging to a Token Family.

### 2. Strict Refresh Token Rotation (RTR) & Family Cascade (RFC 6749)
- `POST /oauth/token` (grant_type: `refresh_token`):
  - Receives `refresh_token`.
  - **Normal Rotation:** If active, marks token as `used`, issues a new `access_token` and `refresh_token` under the same family.
  - **Reuse Detection Trap:** If the token is already `used`, instantly revokes **all** tokens in that token family and returns `400 Bad Request` with top-level `{"error": "invalid_grant"}`.

### 3. Discovery & JWKS Key Set (RFC 8414 & RFC 7517)
- `GET /.well-known/openid-configuration`: Returns OIDC discovery metadata (issuer, token_endpoint, jwks_uri, response_types_supported, grant_types_supported).
- `GET /.well-known/jwks.json`: Returns the active public key set containing `kid`, `kty="RSA"`, `alg="RS256"`, `use="sig"`, `n`, `e`.

### 4. Token Introspection & Revocation (RFC 7662 & RFC 7009)
- `POST /oauth/introspect`: Receives `token` and returns `{"active": true, "scope": "...", "sub": "...", "exp": ...}` or `{"active": false}` if revoked/expired.
- `POST /oauth/revoke`: Receives `token` and `token_type_hint`. Revokes the token and returns `200 OK` with `{"ok": true}`.

### 5. Client Credentials & Device Flow (RFC 6749 & RFC 8628)
- `POST /oauth/token` (grant_type: `client_credentials`): Mints a machine-to-machine scoped access token.
- `POST /oauth/device/code`: Issues `device_code`, `user_code`, `verification_uri`, and `interval`.

## Verification
Run public tests via `pytest /app/tests/public_test.py`.
The sealed hidden verifier runs comprehensive multi-grant tests, cryptographic JWKS assertions, and cascading replay detection attacks.
