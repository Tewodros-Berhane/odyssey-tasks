import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "/app/oauth.db")

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    # TODO: Design and initialize your database tables here
    pass
