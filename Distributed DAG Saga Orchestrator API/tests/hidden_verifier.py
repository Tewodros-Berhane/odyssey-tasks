import pytest
import time
import threading
import uvicorn
from fastapi.testclient import TestClient
from fastapi import FastAPI, Request, HTTPException
from main import app
from database import init_db

dummy_app = FastAPI()
events = []

@dummy_app.post("/target/{step_id}/execute")
async def target_exec(step_id: str, request: Request):
    payload = await request.json()
    events.append(f"exec_{step_id}")
    if payload.get("should_fail"):
        raise HTTPException(status_code=500)
    return {"ok": True}

@dummy_app.post("/target/{step_id}/compensate")
async def target_comp(step_id: str):
    events.append(f"comp_{step_id}")
    return {"ok": True}

def run_dummy():
    uvicorn.run(dummy_app, host="127.0.0.1", port=8001, log_level="critical")

@pytest.fixture(scope="session", autouse=True)
def setup_dummy_server():
    t = threading.Thread(target=run_dummy, daemon=True)
    t.start()
    time.sleep(1)
    yield

@pytest.fixture(autouse=True)
def clean():
    events.clear()
    init_db()

client = TestClient(app)

def test_forward_topological_execution():
    saga_id = "saga_happy"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "A", "dependencies": [], "execute_url": "http://127.0.0.1:8001/target/A/execute", "compensate_url": "http://127.0.0.1:8001/target/A/compensate", "payload": {}},
            {"id": "B", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8001/target/B/execute", "compensate_url": "http://127.0.0.1:8001/target/B/compensate", "payload": {}},
            {"id": "C", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8001/target/C/execute", "compensate_url": "http://127.0.0.1:8001/target/C/compensate", "payload": {}},
            {"id": "D", "dependencies": ["B", "C"], "execute_url": "http://127.0.0.1:8001/target/D/execute", "compensate_url": "http://127.0.0.1:8001/target/D/compensate", "payload": {}}
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    
    for _ in range(20):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "completed": break
        time.sleep(0.5)
        
    assert res.json()["status"] == "completed"
    assert events[0] == "exec_A"
    assert set(events[1:3]) == {"exec_B", "exec_C"}
    assert events[3] == "exec_D"

def test_failure_and_reverse_compensation():
    saga_id = "saga_fail"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "A", "dependencies": [], "execute_url": "http://127.0.0.1:8001/target/A/execute", "compensate_url": "http://127.0.0.1:8001/target/A/compensate", "payload": {}},
            {"id": "B", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8001/target/B/execute", "compensate_url": "http://127.0.0.1:8001/target/B/compensate", "payload": {"should_fail": True}},
            {"id": "C", "dependencies": ["B"], "execute_url": "http://127.0.0.1:8001/target/C/execute", "compensate_url": "http://127.0.0.1:8001/target/C/compensate", "payload": {}}
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    
    for _ in range(20):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "compensated": break
        time.sleep(0.5)
        
    assert events == ["exec_A", "exec_B", "comp_A"]

def test_complex_partial_failure():
    saga_id = "saga_complex"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "X", "dependencies": [], "execute_url": "http://127.0.0.1:8001/target/X/execute", "compensate_url": "http://127.0.0.1:8001/target/X/compensate", "payload": {}},
            {"id": "Y", "dependencies": ["X"], "execute_url": "http://127.0.0.1:8001/target/Y/execute", "compensate_url": "http://127.0.0.1:8001/target/Y/compensate", "payload": {}},
            {"id": "Z", "dependencies": ["X"], "execute_url": "http://127.0.0.1:8001/target/Z/execute", "compensate_url": "http://127.0.0.1:8001/target/Z/compensate", "payload": {"should_fail": True}}
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    
    for _ in range(20):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "compensated": break
        time.sleep(0.5)
        
    assert events[0] == "exec_X"
    assert set(events[1:3]) == {"exec_Y", "exec_Z"}
    assert events[3] == "comp_Y"
    assert events[4] == "comp_X"

def test_invalid_dependencies_rejected():
    payload = {
        "saga_id": "bad",
        "steps": [
            {"id": "A", "dependencies": ["MISSING"], "execute_url": "http://1", "compensate_url": "http://2", "payload": {}}
        ]
    }
    r = client.post("/api/v1/sagas", json=payload)
    assert r.status_code == 400

def test_database_integrity():
    from database import get_db
    conn = get_db()
    rows = conn.execute("SELECT id, status FROM steps WHERE saga_id = 'saga_complex'").fetchall()
    statuses = {row["id"]: row["status"] for row in rows}
    assert statuses.get("X") == "compensated"
    assert statuses.get("Y") == "compensated"
    assert statuses.get("Z") == "failed"