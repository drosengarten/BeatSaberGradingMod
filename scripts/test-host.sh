#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$ROOT/host" -B "$ROOT/host/build"
cmake --build "$ROOT/host/build"
"$ROOT/host/build/cutaccuracy_tests"
