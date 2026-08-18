#!/usr/bin/env bash
set -e

cat << 'EOF' > /app/main.py
import json
import asyncio
import httpx
from fastapi import FastAPI, HTTPException, BackgroundTasks
from models import SagaCreate
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

async def run_saga(saga_id: str):
    conn = get_db()
    async with httpx.AsyncClient() as client:
        while True:
            conn.execute("BEGIN IMMEDIATE")
            steps = conn.execute("SELECT * FROM steps WHERE saga_id = ?", (saga_id,)).fetchall()
            
            failed_steps = [s for s in steps if s["status"] == "failed"]
            if failed_steps:
                conn.execute("UPDATE sagas SET status = 'compensating' WHERE id = ?", (saga_id,))
                conn.execute("COMMIT")
                await compensate_saga(saga_id, client)
                return

            pending_steps = [s for s in steps if s["status"] == "pending"]
            if not pending_steps:
                executing = [s for s in steps if s["status"] == "executing"]
                if not executing:
                    conn.execute("UPDATE sagas SET status = 'completed' WHERE id = ?", (saga_id,))
                    conn.execute("COMMIT")
                    return
                conn.execute("COMMIT")
                await asyncio.sleep(0.2)
                continue
            
            ready_steps = []
            for p in pending_steps:
                deps = json.loads(p["dependencies"])
                deps_completed = True
                for d in deps:
                    d_row = next((x for x in steps if x["id"] == d), None)
                    if not d_row or d_row["status"] != "completed":
                        deps_completed = False
                        break
                if deps_completed:
                    ready_steps.append(p)
            
            if not ready_steps:
                conn.execute("COMMIT")
                await asyncio.sleep(0.2)
                continue
                
            for r in ready_steps:
                conn.execute("UPDATE steps SET status = 'executing' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
            conn.execute("COMMIT")
            
            async def exec_step(r):
                payload = json.loads(r["payload"])
                try:
                    resp = await client.post(r["execute_url"], json=payload, timeout=5.0)
                    resp.raise_for_status()
                    conn.execute("UPDATE steps SET status = 'completed' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
                except Exception:
                    conn.execute("UPDATE steps SET status = 'failed' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
            
            await asyncio.gather(*(exec_step(r) for r in ready_steps))

async def compensate_saga(saga_id: str, client):
    conn = get_db()
    while True:
        conn.execute("BEGIN IMMEDIATE")
        steps = conn.execute("SELECT * FROM steps WHERE saga_id = ?", (saga_id,)).fetchall()
        completed_steps = [s for s in steps if s["status"] == "completed"]
        
        if not completed_steps:
            conn.execute("UPDATE sagas SET status = 'compensated' WHERE id = ?", (saga_id,))
            conn.execute("COMMIT")
            return
            
        ready_to_compensate = []
        for c in completed_steps:
            is_leaf = True
            for other in steps:
                if other["status"] in ("completed", "compensating"):
                    deps = json.loads(other["dependencies"])
                    if c["id"] in deps:
                        is_leaf = False
                        break
            if is_leaf:
                ready_to_compensate.append(c)
                
        for r in ready_to_compensate:
            conn.execute("UPDATE steps SET status = 'compensating' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
        conn.execute("COMMIT")
        
        async def comp_step(r):
            try:
                await client.post(r["compensate_url"], json={}, timeout=5.0)
            except Exception:
                pass 
            conn.execute("UPDATE steps SET status = 'compensated' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
            
        await asyncio.gather(*(comp_step(r) for r in ready_to_compensate))


@app.post("/api/v1/sagas", status_code=202)
async def create_saga(payload: SagaCreate, background_tasks: BackgroundTasks):
    conn = get_db()
    with conn:
        step_ids = {s.id for s in payload.steps}
        for s in payload.steps:
            for d in s.dependencies:
                if d not in step_ids:
                    raise HTTPException(status_code=400, detail="Invalid dependency")

        conn.execute("INSERT INTO sagas (id, status) VALUES (?, 'running')", (payload.saga_id,))
        for s in payload.steps:
            conn.execute(
                "INSERT INTO steps (saga_id, id, dependencies, execute_url, compensate_url, payload, status) VALUES (?, ?, ?, ?, ?, ?, 'pending')",
                (payload.saga_id, s.id, json.dumps(s.dependencies), s.execute_url, s.compensate_url, json.dumps(s.payload))
            )
            
    background_tasks.add_task(run_saga, payload.saga_id)
    return {"saga_id": payload.saga_id, "status": "running"}

@app.get("/api/v1/sagas/{saga_id}")
async def get_saga(saga_id: str):
    conn = get_db()
    row = conn.execute("SELECT status FROM sagas WHERE id = ?", (saga_id,)).fetchone()
    if not row:
        raise HTTPException(status_code=404)
    return {"saga_id": saga_id, "status": row["status"]}
EOF