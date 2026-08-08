# CutAccuracy v0.10 Option B patch

## What changed

v0.10 implements the requested Option B behavior: CutAccuracy is no longer only an overlay. It overwrites Beat Saber's built-in score numerator and max-score denominator after each tracked scoring event so the built-in result/select-screen percentage follows CutAccuracy LEVEL ACC.

## Consistency rule

If CutAccuracy LEVEL ACC is 95%, the mod writes CutAccuracy's rounded earned score into `ScoreController._multipliedScore` and CutAccuracy's rounded max score into `ScoreController._immediateMaxPossibleMultipliedScore`.

This avoids the failure mode where Beat Saber would display custom raw points over a vanilla 115-point denominator:

```text
Wrong: 9500 custom points / 11500 vanilla max = 82.61%
Right: round(9500) / round(10000) = 95.00%
```

Example:

```text
CutAccuracy custom earned/max = 9500 / 10000 = 95%
Stored built-in score         = 9500
Stored built-in max           = 10000
Built-in displayed percent    = 9500 / 10000 = 95%
```

## Files changed

- `include/CutAccuracy/Scoring.hpp` adds true-internal custom score/max helpers and keeps the legacy vanilla projection helper only as a fallback test utility.
- `include/CutAccuracy/Stats.hpp` exposes session aggregate earned/max values.
- `src/Quest/QuestHooks.cpp` patches `ScoreModel.GetNoteScoreDefinition` and synchronizes both `ScoreController._multipliedScore` and `_immediateMaxPossibleMultipliedScore` after every tracked score event.
- `src/Quest/QuestState.cpp/.hpp` adds diagnostics for built-in score overrides.
- `tests/test_core.cpp` adds anti-100/115 regression tests.
- `docs/SCORING_SPEC.md` and `docs/HUD_SPEC.md` document the built-in score override.

## Local audit result

- Host tests: PASS.
- Strict warning build: PASS.
- Release/NDEBUG build: PASS.
- ASan/UBSan sanitizer build: PASS.
- Host CMake build: PASS.
- Metadata/static policy checks: PASS.

A real Quest qmod compile and headset runtime test are still required because this environment lacks QPM/Android NDK/Quest hardware.

## Caution

This intentionally changes Beat Saber's built-in saved/local score path. You stated this is only for local use and leaderboards are irrelevant; treat this as an offline/custom-scoring build.
