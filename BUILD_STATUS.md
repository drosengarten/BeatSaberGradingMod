# Build status — v0.11

## What I attempted locally

- Audited v0.9 and patched v0.11 for integration hygiene.
- Added full-size support for arc endpoints and chain heads as 100-max notes.
- Added fixed 20/0 scoring for chain links.
- Re-ran the host test suite.
- Re-ran the host test suite with AddressSanitizer and UndefinedBehaviorSanitizer.
- Checked whether Quest build tools were available in this environment.

## Local results

- Host tests: **PASS**.
- Sanitizer host tests: **PASS**.
- Static integration checks: **PASS** for object scoring policy, chain-link fixed scoring, component-row isolation, combo-weighted level scoring, dot-note handling, live Combo re-anchoring on HUD updates, flying-score refresh interception, event cleanup, and unknown-speed validation handling.
- Quest `.qmod` build: **not possible in this environment** because `qpm` and the Android NDK `aarch64-linux-android-clang++` toolchain are not installed.

## v0.11 patches over v0.4

- Replaced the old "normal notes only" policy with an explicit object policy:
  - full-size notes and supported arc/chain-head combined types: max 100;
  - chain links and chain-link/arc-head combined type: fixed 20/0;
  - hazards, bombs, walls, `NoScore`, `Ignore`, and unknown/unhittable objects: ignored for raw/level accuracy.
- Added `ScoreObjectRule` / `scoreObjectRuleForScoringType`.
- Added weighted/fixed stat accumulation:
  - `addWeighted` for full-size 100-max objects;
  - `addFixed` for fixed chain links;
  - `addMissWeighted` for zeroes with a non-100 max.
- Component rows now use only full-size cube-model samples, so chain links do not distort Upper/Lower/Before/After/Speed rows.
- Added fixed flying score formatting for chain links.
- Updated docs and tests for the new policy.

## Still requires headset validation

1. Confirm the panel sits above Combo with your chosen HUD distance.
2. Confirm full-size arc endpoints and chain heads generate usable pending cut geometry.
3. Confirm chain links display `20` on hit and count as 20/0 in RAW/LEVEL ACC.
4. Confirm hazards/bombs/walls/NoScore/Ignore do not change RAW ACC or LEVEL ACC.
5. Confirm traversal-missing count is zero or rare on ordinary cuts.
6. Confirm the stock flying number remains replaced by the custom score.

## Recommended Quest target

Beat Saber `1.40.8_7379` through MBF, with the 1.40.8 Quest mod stack.

## v0.11 audit patches over v0.5

- Updated qpm/package versions to `0.11.0`; v0.5 still carried stale 0.1/0.2 metadata.
- Added explicit custom-types delegate include for the ScoreController event delegate.
- Added ScoreController `OnDestroy` cleanup for the scoring-finished delegate.
- Re-anchors the HUD to the live Combo RectTransform on every HUD update, not only on Combo Start.
- Increments the unknown-scoring-type counter when excluded unknown types are encountered.

- Release-mode host test audit no longer depends on C `assert`; checks remain active when `NDEBUG` is set.
