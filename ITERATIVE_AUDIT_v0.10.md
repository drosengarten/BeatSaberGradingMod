# CutAccuracy v0.10 iterative audit

## Summary

I audited v0.9, found a conceptual Option B mistake, patched it, re-ran the checks, found stale documentation from the prior projection approach, patched that too, and then re-ran the full off-headset check set with no further issues found.

## Iteration 1 — v0.9 audit

### Issue found

v0.9 made the built-in displayed percentage match CutAccuracy by projecting the CutAccuracy ratio into Beat Saber's vanilla max-score space:

```text
builtinScore = round(CutAccuracy_LEVEL_ACC * vanillaMax)
```

That avoided the 100/115 percentage error, but it did **not** truly overwrite the internal score denominator. Beat Saber still retained the vanilla max-score model and the score was only a vanilla-compatible projection.

### Patch

v0.10 changes this to true-internal mode:

```text
builtinScore = round(CutAccuracy_LEVEL_EARNED)
builtinMax   = round(CutAccuracy_LEVEL_MAX)
```

It also hooks `ScoreModel.GetNoteScoreDefinition` and mutates Beat Saber's score definitions so the built-in max-score graph uses CutAccuracy's object maxima:

```text
Full-size notes / chain heads: 100 max
Chain links:                   20 max
Excluded/hazards:               0 max / ignored
```

## Iteration 2 — v0.10 audit

### Issue found

Some documentation still described the v0.9 vanilla-projection route even though the code had been changed to true-internal mode.

### Patch

Updated README, scoring spec, HUD spec, Option B patch notes, and final-check report so they consistently describe the true-internal score/max override.

## Iteration 3 — final check

No further off-headset issues were found.

## Checks run

```text
Host shell-script test build: PASS
Strict -Wall -Wextra -Wpedantic -Werror build: PASS
Release -DNDEBUG build: PASS
ASan/UBSan sanitizer build: PASS
Host CMake build: PASS
Version/metadata sanity: PASS
Static policy checks: PASS
Package re-unpack inspection: PASS
```

## Still not verified here

A real Quest `.qmod` build and headset runtime test are still required. This environment does not include QPM, the Android NDK cross-compiler, or a connected Quest.

## Expected behavior after v0.10

If CutAccuracy's level total is:

```text
9500 / 10000 = 95.00%
```

then Beat Saber should also be driven toward:

```text
built-in score = 9500
built-in max   = 10000
built-in %     = 95.00%
```

not:

```text
9500 / 11500 = 82.61%
```
