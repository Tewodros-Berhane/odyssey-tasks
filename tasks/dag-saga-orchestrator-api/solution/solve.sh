#!/usr/bin/env bash
set -e

cat << 'EOF' > /app/database.py
import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "/app/sagas.db")

def get_db():
    conn = sqlite3.connect(DB_PATH, timeout=10.0, isolation_level=None)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    with conn:
        conn.executescript("""
        CREATE TABLE IF NOT EXISTS sagas (
            id TEXT PRIMARY KEY,
            status TEXT NOT NULL, -- 'pending', 'running', 'paused', 'completed', 'compensating', 'compensated', 'compensation_failed'
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS steps (
            saga_id TEXT NOT NULL,
            id TEXT NOT NULL,
            dependencies TEXT NOT NULL,
            execute_url TEXT NOT NULL,
            compensate_url TEXT NOT NULL,
            payload TEXT NOT NULL,
            retry_policy TEXT,
            status TEXT NOT NULL,
            PRIMARY KEY (saga_id, id)
        );
        CREATE TABLE IF NOT EXISTS journal (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            saga_id TEXT NOT NULL,
            from_status TEXT,
            to_status TEXT NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        """)
EOF

cat << 'EOF' > /app/main.py
import json
import asyncio
import httpx
from fastapi import FastAPI, BackgroundTasks
from fastapi.responses import JSONResponse
from models import SagaCreate
from database import init_db, get_db

app = FastAPI()

@app.on_event("startup")
def startup():
    init_db()

def log_journal(conn, saga_id: str, from_status: str, to_status: str):
    conn.execute("INSERT INTO journal (saga_id, from_status, to_status) VALUES (?, ?, ?)",
                 (saga_id, from_status, to_status))

def has_cycle(steps):
    graph = {s.id: list(s.dependencies) for s in steps}
    visited = {}
    
    def dfs(node):
        if visited.get(node) == 1:
            return True
        if visited.get(node) == 2:
            return False
        visited[node] = 1
        for dep in graph.get(node, []):
            if dfs(dep):
                return True
        visited[node] = 2
        return False
        
    for node in graph:
        if dfs(node):
            return True
    return False

async def run_saga(saga_id: str):
    conn = get_db()
    async with httpx.AsyncClient() as client:
        while True:
            conn.execute("BEGIN IMMEDIATE")
            saga_row = conn.execute("SELECT status FROM sagas WHERE id = ?", (saga_id,)).fetchone()
            if not saga_row:
                conn.execute("COMMIT")
                return
            if saga_row["status"] == "paused":
                conn.execute("COMMIT")
                await asyncio.sleep(0.3)
                continue

            steps = conn.execute("SELECT * FROM steps WHERE saga_id = ?", (saga_id,)).fetchall()
            failed_steps = [s for s in steps if s["status"] == "failed"]
            
            if failed_steps:
                log_journal(conn, saga_id, saga_row["status"], "compensating")
                conn.execute("UPDATE sagas SET status = 'compensating', updated_at = CURRENT_TIMESTAMP WHERE id = ?", (saga_id,))
                conn.execute("COMMIT")
                await compensate_saga(saga_id, client)
                return

            pending_steps = [s for s in steps if s["status"] == "pending"]
            if not pending_steps:
                executing = [s for s in steps if s["status"] == "executing"]
                if not executing:
                    log_journal(conn, saga_id, saga_row["status"], "completed")
                    conn.execute("UPDATE sagas SET status = 'completed', updated_at = CURRENT_TIMESTAMP WHERE id = ?", (saga_id,))
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
                retry_cfg = json.loads(r["retry_policy"]) if r["retry_policy"] else {}
                max_retries = retry_cfg.get("max_retries", 0)
                backoff_sec = retry_cfg.get("backoff_sec", 0.1)
                timeout_sec = retry_cfg.get("timeout_sec", 5.0)

                attempt = 0
                while attempt <= max_retries:
                    try:
                        resp = await client.post(r["execute_url"], json=payload, timeout=timeout_sec)
                        resp.raise_for_status()
                        conn.execute("UPDATE steps SET status = 'completed' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
                        return
                    except Exception:
                        attempt += 1
                        if attempt <= max_retries:
                            await asyncio.sleep(backoff_sec * (2 ** (attempt - 1)))
                        else:
                            conn.execute("UPDATE steps SET status = 'failed' WHERE saga_id = ? AND id = ?", (saga_id, r["id"]))
                            return

            await asyncio.gather(*(exec_step(r) for r in ready_steps))

async def compensate_saga(saga_id: str, client):
    conn = get_db()
    while True:
        conn.execute("BEGIN IMMEDIATE")
        steps = conn.execute("SELECT * FROM steps WHERE saga_id = ?", (saga_id,)).fetchall()
        completed_steps = [s for s in steps if s["status"] == "completed"]

        if not completed_steps:
            log_journal(conn, saga_id, "compensating", "compensated")
            conn.execute("UPDATE sagas SET status = 'compensated', updated_at = CURRENT_TIMESTAMP WHERE id = ?", (saga_id,))
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
    step_ids = {s.id for s in payload.steps}
    for s in payload.steps:
        for d in s.dependencies:
            if d not in step_ids:
                return JSONResponse(status_code=400, content={"error": "invalid_dependency"})

    if has_cycle(payload.steps):
        return JSONResponse(status_code=400, content={"error": "cyclic_dependency_detected"})

    conn = get_db()
    with conn:
        conn.execute("INSERT INTO sagas (id, status) VALUES (?, 'running')", (payload.saga_id,))
        log_journal(conn, payload.saga_id, None, "running")
        for s in payload.steps:
            retry_json = json.dumps(s.retry_policy.model_dump()) if s.retry_policy else None
            conn.execute(
                "INSERT INTO steps (saga_id, id, dependencies, execute_url, compensate_url, payload, retry_policy, status) VALUES (?, ?, ?, ?, ?, ?, ?, 'pending')",
                (payload.saga_id, s.id, json.dumps(s.dependencies), s.execute_url, s.compensate_url, json.dumps(s.payload), retry_json)
            )

    background_tasks.add_task(run_saga, payload.saga_id)
    return {"saga_id": payload.saga_id, "status": "running"}

@app.get("/api/v1/sagas/{saga_id}")
async def get_saga(saga_id: str):
    conn = get_db()
    row = conn.execute("SELECT * FROM sagas WHERE id = ?", (saga_id,)).fetchone()
    if not row:
        return JSONResponse(status_code=404, content={"error": "not_found"})
    return {"saga_id": saga_id, "status": row["status"]}

@app.get("/api/v1/sagas/{saga_id}/steps")
async def get_saga_steps(saga_id: str):
    conn = get_db()
    rows = conn.execute("SELECT id, status FROM steps WHERE saga_id = ?", (saga_id,)).fetchall()
    return {"steps": [{"id": r["id"], "status": r["status"]} for r in rows]}

@app.get("/api/v1/sagas/{saga_id}/journal")
async def get_saga_journal(saga_id: str):
    conn = get_db()
    rows = conn.execute("SELECT from_status, to_status, timestamp FROM journal WHERE saga_id = ? ORDER BY id ASC", (saga_id,)).fetchall()
    return {"journal": [{"from_status": r["from_status"], "to_status": r["to_status"], "timestamp": r["timestamp"]} for r in rows]}

@app.post("/api/v1/sagas/{saga_id}/pause")
async def pause_saga(saga_id: str):
    conn = get_db()
    with conn:
        conn.execute("UPDATE sagas SET status = 'paused' WHERE id = ?", (saga_id,))
        log_journal(conn, saga_id, "running", "paused")
    return {"ok": True}

@app.post("/api/v1/sagas/{saga_id}/resume")
async def resume_saga(saga_id: str):
    conn = get_db()
    with conn:
        conn.execute("UPDATE sagas SET status = 'running' WHERE id = ?", (saga_id,))
        log_journal(conn, saga_id, "paused", "running")
    return {"ok": True}
EOF
