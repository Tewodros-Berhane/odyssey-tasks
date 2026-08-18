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
retry_counts = {}

@dummy_app.post("/target/{step_id}/execute")
async def target_exec(step_id: str, request: Request):
    payload = await request.json()
    events.append(f"exec_{step_id}")
    
    if payload.get("transient_fail"):
        retry_counts[step_id] = retry_counts.get(step_id, 0) + 1
        if retry_counts[step_id] < payload["transient_fail"]:
            raise HTTPException(status_code=503, detail="Service Unavailable")
            
    if payload.get("should_fail"):
        raise HTTPException(status_code=500, detail="Internal Server Error")
        
    return {"ok": True}

@dummy_app.post("/target/{step_id}/compensate")
async def target_comp(step_id: str):
    events.append(f"comp_{step_id}")
    return {"ok": True}

def run_dummy():
    uvicorn.run(dummy_app, host="127.0.0.1", port=8002, log_level="critical")

@pytest.fixture(scope="session", autouse=True)
def setup_dummy_server():
    t = threading.Thread(target=run_dummy, daemon=True)
    t.start()
    time.sleep(1)
    yield

@pytest.fixture(autouse=True)
def clean():
    events.clear()
    retry_counts.clear()
    init_db()

client = TestClient(app)

def test_dag_cycle_and_dependency_validation():
    # 1. Dangling dependency
    payload_dangling = {
        "saga_id": "bad_dep",
        "steps": [
            {"id": "A", "dependencies": ["NON_EXISTENT"], "execute_url": "http://127.0.0.1:8002/target/A/execute", "compensate_url": "http://127.0.0.1:8002/target/A/compensate", "payload": {}}
        ]
    }
    r1 = client.post("/api/v1/sagas", json=payload_dangling)
    assert r1.status_code == 400
    assert "invalid_dependency" in r1.json().get("error", "")

    # 2. Cyclic dependency (A -> B -> C -> A)
    payload_cycle = {
        "saga_id": "bad_cycle",
        "steps": [
            {"id": "A", "dependencies": ["C"], "execute_url": "http://127.0.0.1:8002/target/A/execute", "compensate_url": "http://127.0.0.1:8002/target/A/compensate", "payload": {}},
            {"id": "B", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8002/target/B/execute", "compensate_url": "http://127.0.0.1:8002/target/B/compensate", "payload": {}},
            {"id": "C", "dependencies": ["B"], "execute_url": "http://127.0.0.1:8002/target/C/execute", "compensate_url": "http://127.0.0.1:8002/target/C/compensate", "payload": {}}
        ]
    }
    r2 = client.post("/api/v1/sagas", json=payload_cycle)
    assert r2.status_code == 400
    assert "cyclic_dependency_detected" in r2.json().get("error", "")

def test_forward_topological_execution():
    saga_id = "saga_diamond_forward"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "A", "dependencies": [], "execute_url": "http://127.0.0.1:8002/target/A/execute", "compensate_url": "http://127.0.0.1:8002/target/A/compensate", "payload": {}},
            {"id": "B", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8002/target/B/execute", "compensate_url": "http://127.0.0.1:8002/target/B/compensate", "payload": {}},
            {"id": "C", "dependencies": ["A"], "execute_url": "http://127.0.0.1:8002/target/C/execute", "compensate_url": "http://127.0.0.1:8002/target/C/compensate", "payload": {}},
            {"id": "D", "dependencies": ["B", "C"], "execute_url": "http://127.0.0.1:8002/target/D/execute", "compensate_url": "http://127.0.0.1:8002/target/D/compensate", "payload": {}}
        ]
    }
    r = client.post("/api/v1/sagas", json=payload)
    assert r.status_code == 202
    
    for _ in range(25):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "completed": break
        time.sleep(0.4)
        
    assert res.json()["status"] == "completed"
    assert events[0] == "exec_A"
    assert set(events[1:3]) == {"exec_B", "exec_C"}
    assert events[3] == "exec_D"

def test_retry_policy_with_backoff():
    saga_id = "saga_with_retry"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {
                "id": "R1",
                "dependencies": [],
                "execute_url": "http://127.0.0.1:8002/target/R1/execute",
                "compensate_url": "http://127.0.0.1:8002/target/R1/compensate",
                "payload": {"transient_fail": 3}, # Fails twice, succeeds on 3rd
                "retry_policy": {"max_retries": 3, "backoff_sec": 0.1, "timeout_sec": 5.0}
            }
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    for _ in range(20):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "completed": break
        time.sleep(0.4)
    assert res.json()["status"] == "completed"
    assert retry_counts.get("R1") == 3

def test_failure_and_reverse_topological_compensation():
    saga_id = "saga_fail_complex"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "X", "dependencies": [], "execute_url": "http://127.0.0.1:8002/target/X/execute", "compensate_url": "http://127.0.0.1:8002/target/X/compensate", "payload": {}},
            {"id": "Y", "dependencies": ["X"], "execute_url": "http://127.0.0.1:8002/target/Y/execute", "compensate_url": "http://127.0.0.1:8002/target/Y/compensate", "payload": {}},
            {"id": "Z", "dependencies": ["X"], "execute_url": "http://127.0.0.1:8002/target/Z/execute", "compensate_url": "http://127.0.0.1:8002/target/Z/compensate", "payload": {"should_fail": True}}
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    for _ in range(25):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "compensated": break
        time.sleep(0.4)
        
    assert res.json()["status"] == "compensated"
    assert events[0] == "exec_X"
    assert set(events[1:3]) == {"exec_Y", "exec_Z"}
    assert events[3] == "comp_Y"
    assert events[4] == "comp_X"

def test_pause_resume_and_journal_audit():
    saga_id = "saga_journal_audit"
    payload = {
        "saga_id": saga_id,
        "steps": [
            {"id": "J1", "dependencies": [], "execute_url": "http://127.0.0.1:8002/target/J1/execute", "compensate_url": "http://127.0.0.1:8002/target/J1/compensate", "payload": {}}
        ]
    }
    client.post("/api/v1/sagas", json=payload)
    for _ in range(15):
        res = client.get(f"/api/v1/sagas/{saga_id}")
        if res.json()["status"] == "completed": break
        time.sleep(0.3)
    
    # Audit Journal
    r_j = client.get(f"/api/v1/sagas/{saga_id}/journal")
    assert r_j.status_code == 200
    journal = r_j.json()["journal"]
    assert len(journal) >= 2
    transitions = [entry["to_status"] for entry in journal]
    assert "running" in transitions
    assert "completed" in transitions
