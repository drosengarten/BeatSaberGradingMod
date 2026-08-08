# CutAccuracy v0.11

Quest standalone Beat Saber 1.40.8 source package for the custom cut-accuracy metric.

## Scoring summary

CutAccuracy separates **quality display** from **object max score**:

- Full-size scoreable notes are scored out of **100**.
- Chain links are fixed **20 on hit / 0 on miss or bad cut**.
- Hazards, bombs, walls, `NoScore`, `Ignore`, and unknown/unhittable objects are ignored for `RAW ACC` and `LEVEL ACC`.

## Full-size note model

Full-size notes include ordinary arrow notes, dot notes, arc endpoints, chain heads, and supported combined full-size arc/chain-head types. Each full-size note is scored out of 100:

- First/upper mini-note volume balance: 25
- Second/lower mini-note volume balance: 25
- Before-swing angle: 20
- After-swing angle: 20
- Through-note traversal speed: 10

Dot notes use the actual saber travel direction at the cut to choose the temporary mini-note split axis.

## Chain links

Chain links do not use the cube/mini-note model. A successful chain link contributes 20/20. A missed or bad-cut chain link contributes 0/20.

## HUD values

The persistent HUD displays two headline values:

- `LEVEL ACC`: combo-weighted custom level accuracy.
- `RAW ACC`: physical cut accuracy / chain-link completion, not combo-weighted.

The level score math is:

```text
rawEarned   += objectScore
rawMax      += objectMax

levelEarned += objectScore * actualMultiplier
levelMax    += objectMax * maxPossibleMultiplier
```

Examples:

```text
Perfect full note at x8:       100 * 8 / 100 * 8
Perfect chain link at x8:       20 * 8 /  20 * 8
Missed chain link at x8:         0 * 8 /  20 * 8
Bomb/wall/NoScore/Ignore:        ignored, no denominator
```

## v0.11 runtime improvements

- hazards/unhittable objects are ignored for raw accuracy;
- `NoScore` and `Ignore` are excluded from all custom accuracy denominators;
- full-size arc endpoints and chain heads are supported as 100-max notes;
- chain links are fixed 20/0 objects;
- chain links do not pollute the Upper/Lower/Before/After/Speed component rows;
- the panel remains attached above the live Combo UI;
- the floating score replacement is reapplied inside `FlyingScoreEffect::RefreshScore`.

## Build status

The host suite passes in this environment, including sanitizer checks. A `.qmod` was not built here because the environment does not include QPM or the Android NDK cross-compiler.


## v0.11 built-in score override

v0.11 is Option B true-internal mode: Beat Saber's score model/max-score
denominator and current score total are rewritten into CutAccuracy's own score
space. If CutAccuracy's current level score is 9500 / 10000, Beat Saber is kept
at 9500 / 10000 too, not 9500 / 11500.

This is a deliberate custom-scoring override, not just an overlay. Treat it as
offline/local until leaderboard interactions are verified.
