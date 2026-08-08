# CutAccuracy v0.8 final iterative check

## Iteration result

I checked v0.6, found issues, patched v0.7, checked again, found two further off-headset hygiene/runtime-risk issues, patched v0.8, and then rechecked v0.8. The final v0.8 check found no further off-headset issues.

## Fixes over v0.7

- Normalized Quest `CMakeLists.txt` so `cmake_minimum_required()` is first and `CMAKE_CXX_STANDARD_REQUIRED` is explicitly `ON`.
- Destroyed the previous HUD panel GameObject inside `ClearHud()` before clearing pointers, preventing duplicate panels if the Combo UI is reinstalled during unusual scene transitions.
- Updated package metadata, docs, and comments to `0.8.0`.

## Verified locally on v0.8

- Host shell-script test build: PASS.
- Strict `-Wall -Wextra -Wpedantic -Werror` build: PASS.
- `-DNDEBUG` release build: PASS.
- ASan/UBSan sanitizer build: PASS.
- Direct host CMake build via `cmake -S host -B build-cmake-host`: PASS.
- Package metadata sanity checks: PASS.
- Static checks for scoring policy, hazards/unhittable exclusion, chain link fixed scoring, Combo HUD anchoring, HUD cleanup, flying-score replacement, and scoring delegate lifecycle: PASS.

## Not verified here

A real Quest `.qmod` build and headset runtime test remain unverified because this runtime does not include QPM, the Android NDK cross-compiler, or a connected Quest.

## Runtime target

Beat Saber Quest standalone `1.40.8_7379`, with bs-cordl `4008.*`.
