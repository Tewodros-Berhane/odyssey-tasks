from fastapi import FastAPI, HTTPException, BackgroundTasks
from models import SagaCreate
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

@app.post("/api/v1/sagas", status_code=202)
async def create_saga(payload: SagaCreate, background_tasks: BackgroundTasks):
    # TODO: Implement Saga DAG scheduling
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/sagas/{saga_id}")
async def get_saga(saga_id: str):
    # TODO: Implement Saga status retrieval
    raise HTTPException(status_code=501, detail="Not Implemented")
