from pydantic import BaseModel
from typing import Optional

class AuthorizeRequest(BaseModel):
    client_id: str
    response_type: str
    code_challenge: str
    code_challenge_method: str
    scope: Optional[str] = "openid"

class TokenRequest(BaseModel):
    grant_type: str
    client_id: Optional[str] = None
    client_secret: Optional[str] = None
    code: Optional[str] = None
    code_verifier: Optional[str] = None
    refresh_token: Optional[str] = None
    device_code: Optional[str] = None
    scope: Optional[str] = None

class IntrospectRequest(BaseModel):
    token: str
    token_type_hint: Optional[str] = None

class RevokeRequest(BaseModel):
    token: str
    token_type_hint: Optional[str] = None

class DeviceCodeRequest(BaseModel):
    client_id: str
    scope: Optional[str] = None
