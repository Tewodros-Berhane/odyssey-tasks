import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "/app/oauth.db")

def get_db():
    conn = sqlite3.connect(DB_PATH, timeout=10.0, isolation_level=None)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    with conn:
        conn.executescript("""
        CREATE TABLE IF NOT EXISTS clients (
            client_id TEXT PRIMARY KEY,
            client_secret TEXT NOT NULL,
            client_name TEXT NOT NULL,
            redirect_uris TEXT NOT NULL,
            grant_types TEXT NOT NULL,
            token_endpoint_auth_method TEXT DEFAULT 'client_secret_post',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS par_requests (
            request_uri TEXT PRIMARY KEY,
            client_id TEXT NOT NULL,
            challenge TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            expires_at TIMESTAMP NOT NULL,
            FOREIGN KEY (client_id) REFERENCES clients(client_id)
        );
        CREATE TABLE IF NOT EXISTS auth_codes (
            code TEXT PRIMARY KEY,
            client_id TEXT NOT NULL,
            challenge TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            dpop_jkt TEXT,
            used INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS tokens (
            token TEXT PRIMARY KEY,
            family_id TEXT NOT NULL,
            token_type TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            dpop_jkt TEXT,
            status TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS device_codes (
            device_code TEXT PRIMARY KEY,
            user_code TEXT NOT NULL UNIQUE,
            client_id TEXT NOT NULL,
            scope TEXT DEFAULT 'openid',
            status TEXT NOT NULL DEFAULT 'pending' -- 'pending', 'approved', 'denied'
        );
        CREATE TABLE IF NOT EXISTS dpop_jti_cache (
            jti TEXT PRIMARY KEY,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS users (
            sub TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            email TEXT NOT NULL,
            email_verified INTEGER DEFAULT 1
        );
        """)
        # Insert default sample user
        conn.execute("INSERT OR IGNORE INTO users (sub, name, email) VALUES ('user_123', 'Odyssey Developer', 'dev@odyssey.com')")
