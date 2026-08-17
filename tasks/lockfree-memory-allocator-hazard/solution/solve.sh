#!/usr/bin/env bash
set -e

echo "=== [Odyssey Oracle] Applying Lock-Free Allocator Reference Solution ==="

cd /app

mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja

echo "=== [Odyssey Oracle] Lock-Free Allocator Reference Built Successfully ==="
