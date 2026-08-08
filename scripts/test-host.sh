#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-host
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  tests/test_core.cpp \
  src/Geometry.cpp src/Stats.cpp src/Presentation.cpp src/Traversal.cpp \
  -o build-host/cutaccuracy_tests
./build-host/cutaccuracy_tests
