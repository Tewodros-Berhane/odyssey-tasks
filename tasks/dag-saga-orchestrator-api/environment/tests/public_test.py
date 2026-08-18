import pytest
from fastapi.testclient import TestClient
from main import app

client = TestClient(app)

def test_root():
    assert app is not None
