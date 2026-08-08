# CutAccuracy v0.11 compile readiness

## Language

CutAccuracy is a native Quest Beat Saber mod written in C++.

- Core/math/test code: standard C++20.
- Quest runtime code: C++20 plus Beat Saber Quest IL2CPP/QPM headers.
- Android target: arm64-v8a shared library, packaged as a `.qmod`.

## Compile hygiene patch over v0.10

v0.11 keeps the v0.10 scoring behavior but adds explicit compile-hygiene includes in the Quest hook translation unit:

- `<functional>` for `std::function` used by the custom-types delegate creation.
- `beatsaber-hook/shared/utils/byref.hpp` for the `ByRef<NoteCutInfo>` hook argument.

These were likely included indirectly by dependency headers, but explicit includes are safer and make the translation unit self-contained.

## Verified locally

This environment can compile and run the host-side C++20 core. It cannot compile the Quest `.so` because `qpm` and the Android NDK are not installed here.

Verified checks:

- `scripts/test-host.sh`
- strict C++20 host build with `-Wall -Wextra -Wpedantic -Werror`
- release/NDEBUG host build
- ASan/UBSan host build
- CMake host build
- package re-unpack sanity

## Quest compile requirement

A real Quest build still requires:

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

If `qpm` and NDK 27 are installed, the project is structured as a normal native C++ Quest mod with `qpm.json`, `CMakeLists.txt`, `mod.template.json`, `src/`, and `include/`.
