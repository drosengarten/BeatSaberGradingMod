# Final language and compile check — CutAccuracy v0.11

## Result

Use CutAccuracy v0.11.

The project is using the correct language for a Quest standalone Beat Saber native mod:

- Quest mod/runtime layer: C++20 native shared library (`libcutaccuracy.so`).
- Core scoring layer: standard C++20.
- Build/package metadata: QPM + CMake + `mod.template.json` for a Scotland2 `.qmod`.
- Target Beat Saber/bs-cordl line: `1.40.8_7379` / `4008.*`.

## Patch made during this check

v0.10 was structurally correct, but one Quest source file relied on likely transitive includes. v0.11 adds explicit includes so the Quest hook translation unit is more self-contained:

```cpp
#include "beatsaber-hook/shared/utils/byref.hpp"
#include <functional>
```

This is for:

- `ByRef<NoteCutInfo>` in the note-cut hook signature.
- `std::function` in the `custom_types::MakeDelegate` call.

No scoring behavior was changed from v0.10.

## Checks actually run here

This environment can compile the host-side C++20 core. It cannot run QPM or the Android NDK because neither is installed in the runtime.

Passed locally:

```text
scripts/test-host.sh                         PASS
g++ strict C++20 -Wall -Wextra -Werror       PASS
g++ release/NDEBUG C++20                     PASS
g++ ASan/UBSan C++20                         PASS
clang++ strict C++20                         PASS
host CMake build                             PASS
package metadata sanity                      PASS
explicit Quest include sanity                PASS
```

## What is still not proven here

The actual Quest `.so` build still must be done on a machine with:

```text
QPM
Android NDK 27
CMake
Ninja
bs-cordl / Beat Saber 1.40.8_7379 dependencies restored by qpm
```

The command sequence remains:

```bash
qpm ndk pin 27
qpm restore
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29
cmake --build build
qpm qmod build
```

## Final answer

The language is correct: native C++20 for Quest/Scotland2. The core C++ compiles here. A full Quest compile cannot be certified in this container because QPM and the Android NDK are unavailable, but v0.11 removes the compile-hygiene issue I found and is the cleanest source package to try in the real Quest toolchain.
