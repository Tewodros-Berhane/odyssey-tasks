#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying Raft Reference Solution ==="

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] Raft Reference Built Successfully ==="
