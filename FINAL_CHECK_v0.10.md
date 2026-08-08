# CutAccuracy v0.10 final check

## Requested behavior

Option B is implemented: Beat Saber's built-in score value and max-score denominator are overridden so the built-in end/select-screen percentage should match CutAccuracy LEVEL ACC instead of treating custom 100-point notes as if they were vanilla 115-point notes.

## Critical anti-100/115 rule

The mod does **not** write raw custom points into Beat Saber's vanilla denominator. It writes CutAccuracy's rounded earned total and rounded max total into Beat Saber's built-in score fields:

```text
builtinScore = round(CutAccuracy_LEVEL_EARNED)
builtinMax   = round(CutAccuracy_LEVEL_MAX)
```

Therefore:

```text
95 custom acc -> approximately 95% built-in display
```

not:

```text
95 / 115 = 82.61%
```

## Local verification

- Host shell-script test build: PASS.
- Strict `-Wall -Wextra -Wpedantic -Werror` build: PASS.
- `-DNDEBUG` release build: PASS.
- ASan/UBSan sanitizer build: PASS.
- Direct host CMake build: PASS.
- Package metadata sanity checks: PASS.
- Static checks for scoring policy, hazards/unhittable exclusion, chain-link fixed scoring, Combo HUD anchoring, HUD cleanup, flying-score replacement, scoring delegate lifecycle, and built-in score override helper: PASS.

## Not verified here

A real Quest `.qmod` build and headset runtime test remain unverified because this runtime does not include QPM, the Android NDK cross-compiler, or a connected Quest.

## Runtime target

Beat Saber Quest standalone `1.40.8_7379`, with bs-cordl `4008.*`.
