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
            status TEXT NOT NULL -- 'running', 'completed', 'compensating', 'compensated'
        );
        CREATE TABLE IF NOT EXISTS steps (
            saga_id TEXT NOT NULL,
            id TEXT NOT NULL,
            dependencies TEXT NOT NULL, -- JSON array
            execute_url TEXT NOT NULL,
            compensate_url TEXT NOT NULL,
            payload TEXT NOT NULL,      -- JSON dict
            status TEXT NOT NULL,       -- 'pending', 'executing', 'completed', 'failed', 'compensating', 'compensated'
            PRIMARY KEY (saga_id, id)
        );
        """)
