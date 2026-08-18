from pydantic import BaseModel
from typing import List, Dict, Any

class StepDefinition(BaseModel):
    id: str
    dependencies: List[str] = []
    execute_url: str
    compensate_url: str
    payload: Dict[str, Any] = {}

class SagaCreate(BaseModel):
    saga_id: str
    steps: List[StepDefinition]