from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.responses import JSONResponse
from models import SagaCreate
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

@app.post("/api/v1/sagas", status_code=202)
async def create_saga(payload: SagaCreate, background_tasks: BackgroundTasks):
    # TODO: Implement Kahn's cycle detection, validation, and async background DAG execution
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/sagas/{saga_id}")
async def get_saga(saga_id: str):
    # TODO: Implement Saga status query
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/sagas/{saga_id}/steps")
async def get_saga_steps(saga_id: str):
    # TODO: Implement step-level execution status retrieval
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.get("/api/v1/sagas/{saga_id}/journal")
async def get_saga_journal(saga_id: str):
    # TODO: Implement append-only state transition journal retrieval
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/sagas/{saga_id}/pause")
async def pause_saga(saga_id: str):
    # TODO: Implement in-flight saga pause control
    raise HTTPException(status_code=501, detail="Not Implemented")

@app.post("/api/v1/sagas/{saga_id}/resume")
async def resume_saga(saga_id: str):
    # TODO: Implement in-flight saga resume control
    raise HTTPException(status_code=501, detail="Not Implemented")
