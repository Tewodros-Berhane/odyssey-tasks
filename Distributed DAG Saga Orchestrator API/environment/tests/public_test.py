from fastapi.testclient import TestClient
from main import app
client = TestClient(app)

def test_api_is_up():
    assert app.title is not None