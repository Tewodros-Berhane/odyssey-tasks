from fastapi import FastAPI, HTTPException
from models import AuthorizeRequest, TokenRequest
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

@app.post("/oauth/authorize")
async def authorize(payload: AuthorizeRequest):
    # TODO: Implement PKCE Authorization Code generation & storage
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/oauth/token")
async def token(payload: TokenRequest):
    # TODO: Implement Token Exchange & RTR Logic
    raise HTTPException(status_code=501, detail="Not Implemented")
