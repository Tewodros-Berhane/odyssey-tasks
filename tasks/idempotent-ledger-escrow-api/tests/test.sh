#!/usr/bin/env bash
set -e

export LEDGER_DB_PATH="/tmp/test_ledger.db"
rm -f /tmp/test_ledger.db /tmp/test_ledger.db-wal /tmp/test_ledger.db-shm

if [ -f "/tests/hidden_verifier.py" ]; then
    python3 -m pytest /tests/hidden_verifier.py -v --tb=short > /tmp/pytest_results.log 2>&1 || true
elif [ -f "tests/hidden_verifier.py" ]; then
    python3 -m pytest tests/hidden_verifier.py -v --tb=short > /tmp/pytest_results.log 2>&1 || true
elif [ -f "/app/tests/hidden_verifier.py" ]; then
    python3 -m pytest /app/tests/hidden_verifier.py -v --tb=short > /tmp/pytest_results.log 2>&1 || true
else
    echo "Could not find hidden_verifier.py"
    mkdir -p verifier /tmp/verifier
    echo "0.0" > verifier/reward.txt
    echo "0.0" > reward.txt
    echo '{"reward": 0.0}' > reward.json
    exit 1
fi

python3 - << 'EOF'
import re, json, os, sys

output = ""
if os.path.exists("/tmp/pytest_results.log"):
    with open("/tmp/pytest_results.log") as f:
        output = f.read()

suites = {
    "test_crud_and_initialization": 0.15,
    "test_idempotency_and_hash_conflicts": 0.20,
    "test_escrow_hold_capture_partial_and_void": 0.25,
    "test_auto_expiration_and_temporal_release": 0.15,
    "test_double_entry_zero_sum_invariants": 0.15,
    "test_hmac_signature_and_clock_skew": 0.10
}

score = 0.0
passed_tests = re.findall(r"hidden_verifier\.py::(\w+)\s+PASSED", output)

for test_name, weight in suites.items():
    if test_name in passed_tests:
        score += weight

score = round(score, 4)
print(f"FINAL CALCULATED REWARD: {score}")

# Create all candidate reward directories
for d in ["verifier", "/tmp/verifier", "/logs/verifier", "/app", "."]:
    try:
        os.makedirs(d, exist_ok=True)
    except Exception:
        pass

# Write reward.txt and reward.json in all standard locations
targets = [
    "verifier/reward.txt",
    "reward.txt",
    "/tmp/verifier/reward.txt",
    "/tmp/reward.txt",
    "/logs/verifier/reward.txt",
    "/app/reward.txt"
]

for target in targets:
    try:
        with open(target, "w") as f:
            f.write(str(score))
    except Exception:
        pass

json_targets = [
    "reward.json",
    "verifier/reward.json",
    "/tmp/verifier/reward.json",
    "/tmp/reward.json",
    "/logs/verifier/reward.json",
    "/app/reward.json"
]

for jtarget in json_targets:
    try:
        with open(jtarget, "w") as f:
            json.dump({"reward": score, "passed": passed_tests}, f)
    except Exception:
        pass

EOF

echo "Verifier finished successfully."
