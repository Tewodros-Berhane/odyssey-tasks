from pydantic import BaseModel
from typing import Optional

class ParRequest(BaseModel):
    client_id: str
    response_type: str = "code"
    code_challenge: str
    code_challenge_method: str = "S256"
    redirect_uri: Optional[str] = None
    scope: Optional[str] = "openid"

class AuthorizeRequest(BaseModel):
    client_id: Optional[str] = None
    response_type: Optional[str] = "code"
    code_challenge: Optional[str] = None
    code_challenge_method: Optional[str] = "S256"
    scope: Optional[str] = "openid"
    request_uri: Optional[str] = None

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
