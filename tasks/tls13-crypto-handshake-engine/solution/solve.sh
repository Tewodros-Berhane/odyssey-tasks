#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying TLS 1.3 Reference Solution ==="

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] TLS 1.3 Reference Built Successfully ==="
