from pydantic import BaseModel
from typing import List, Dict, Any, Optional

class RetryPolicy(BaseModel):
    max_retries: int = 0
    backoff_sec: float = 0.1
    timeout_sec: float = 5.0

class SagaStep(BaseModel):
    id: str
    dependencies: List[str] = []
    execute_url: str
    compensate_url: str
    payload: Dict[str, Any] = {}
    retry_policy: Optional[RetryPolicy] = None

class SagaCreate(BaseModel):
    saga_id: str
    steps: List[SagaStep]
