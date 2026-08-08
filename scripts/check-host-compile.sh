#!/usr/bin/env bash
set -euo pipefail
cxx=${CXX:-g++}
$cxx -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -Iinclude tests/test_core.cpp \
  src/Geometry.cpp src/Stats.cpp src/Presentation.cpp src/Traversal.cpp \
  -o /tmp/cutaccuracy_host_strict
/tmp/cutaccuracy_host_strict
$cxx -std=c++20 -DNDEBUG -O2 -Wall -Wextra -Wpedantic -Werror \
  -Iinclude tests/test_core.cpp \
  src/Geometry.cpp src/Stats.cpp src/Presentation.cpp src/Traversal.cpp \
  -o /tmp/cutaccuracy_host_release
/tmp/cutaccuracy_host_release
