#!/usr/bin/env bash
set -e

echo "=== Running Sealed Verification Pipeline ==="

export LEDGER_DB_PATH="/tmp/test_ledger.db"
rm -f /tmp/test_ledger.db /tmp/test_ledger.db-wal /tmp/test_ledger.db-shm

python3 -m pytest /tests/hidden_verifier.py -v --tb=short > /tmp/pytest_results.log 2>&1
PYTEST_EXIT=$?

cat /tmp/pytest_results.log

python3 - << 'EOF'
import re, json, sys

with open("/tmp/pytest_results.log") as f:
    output = f.read()

# Parse weights
suites = {
    "test_crud_and_initialization": 0.15,
    "test_idempotency_and_hash_conflicts": 0.20,
    "test_escrow_hold_capture_partial_and_void": 0.25,
    "test_auto_expiration_and_temporal_release": 0.15,
    "test_double_entry_zero_sum_invariants": 0.15,
    "test_hmac_signature_and_clock_skew": 0.10
}

score = 0.0
passed_tests = re.findall(r"hidden_verifier.py::(\w+)\s+PASSED", output)

for test_name, weight in suites.items():
    if test_name in passed_tests:
        score += weight

score = round(score, 4)
print(f"\n==========================================")
print(f"FINAL CALCULATED REWARD: {score}")
print(f"==========================================")

with open("/tmp/grade_report.json", "w") as f:
    json.dump({"reward": score, "passed": passed_tests}, f)

if score < 0.95:
    sys.exit(1)
sys.exit(0)
EOF

EXIT_CODE=$?
exit $EXIT_CODE