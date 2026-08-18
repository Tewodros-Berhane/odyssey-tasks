#!/usr/bin/env bash
set -e

export DB_PATH="/tmp/test_oauth.db"
rm -f $DB_PATH

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
    "test_authorize_and_issue": 0.20,
    "test_pkce_padding_strictness": 0.15,
    "test_refresh_token_rotation_and_cascade": 0.25,
    "test_jwks_and_oidc_discovery": 0.20,
    "test_token_introspection_and_revocation": 0.10,
    "test_client_credentials_and_device_flow": 0.10
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
