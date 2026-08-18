#!/usr/bin/env bash
set -e

export DB_PATH="/tmp/test_sagas.db"
rm -f $DB_PATH

python3 -m pytest /tests/hidden_verifier.py -v --tb=short > /tmp/pytest_results.log 2>&1 || true

python3 - << 'EOF'
import re, json, sys

with open("/tmp/pytest_results.log") as f:
    output = f.read()

suites = {
    "test_forward_topological_execution": 0.25,
    "test_failure_and_reverse_compensation": 0.25,
    "test_complex_partial_failure": 0.25,
    "test_invalid_dependencies_rejected": 0.15,
    "test_database_integrity": 0.10
}

score = 0.0
passed_tests = re.findall(r"hidden_verifier.py::(\w+)\s+PASSED", output)

for test_name, weight in suites.items():
    if test_name in passed_tests:
        score += weight

score = round(score, 4)
print(f"FINAL CALCULATED REWARD: {score}")

with open("/tmp/grade_report.json", "w") as f:
    json.dump({"reward": score, "passed": passed_tests}, f)

if score < 0.95:
    sys.exit(1)
sys.exit(0)
EOF