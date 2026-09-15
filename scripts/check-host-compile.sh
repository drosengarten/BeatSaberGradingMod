#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
c++ -std=c++20 -I"$ROOT/include" -fsyntax-only "$ROOT/tests/test_core.cpp" "$ROOT/src/Geometry.cpp" "$ROOT/src/Stats.cpp" "$ROOT/src/Presentation.cpp" "$ROOT/src/Traversal.cpp"
