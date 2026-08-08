# CutAccuracy v0.11 scoring spec

## Object policy

CutAccuracy v0.11 counts only scoreable cut objects. Hazards and unhittable/non-score objects are ignored for both `RAW ACC` and `LEVEL ACC`.

| Beat Saber scoring object | Custom max | Mechanism |
|---|---:|---|
| Normal arrow note | 100 | full 100-point cube model |
| Normal dot note | 100 | full cube model, split axis from saber travel |
| ArcHead | 100 | full cube model |
| ArcTail | 100 | full cube model |
| ArcHeadArcTail | 100 | full cube model |
| ChainHead | 100 | full cube model |
| ChainHeadArcTail | 100 | full cube model |
| ChainHeadArcHead | 100 | full cube model, supported defensively |
| ChainHeadArcHeadArcTail | 100 | full cube model, supported defensively |
| ChainLink | 20 | fixed 20 on hit, 0 on miss/bad cut |
| ChainLinkArcHead | 20 | fixed 20 on hit, 0 on miss/bad cut |
| NoScore | 0 | excluded |
| Ignore | 0 | excluded |
| Bombs/walls/hazards | 0 | ignored for accuracy |
| Unknown/unhittable | 0 | ignored for accuracy |

## Full-size note raw score

A full-size scoreable note has a 100-point custom cut score:

- First/upper mini-note volume balance: 25
- Second/lower mini-note volume balance: 25
- Before-swing angle up to 60 degrees: 20
- After-swing angle up to 60 degrees: 20
- Through-note traversal speed: 10

Mini-note score uses the smaller volume fraction on either side of the saber cut plane:

`score = min(V1,V2)/(V1+V2) * 50`

The speed score is:

`10 * min(1, 0.100 / traversalSeconds)`

## Chain links

Chain links intentionally do not use the cube model. They match the vanilla-style fixed-link behavior:

`hit = 20 / 20`

`miss or bad cut = 0 / 20`

Chain links increase the raw/level denominator but do not affect Upper, Lower, Before, After, or Speed component rows.

## Raw accuracy

Raw accuracy exposes physical cut quality and chain-link completion. Misses/bad cuts for scoreable objects remain zeroes:

`rawAccuracy = rawEarned / rawMax * 100`

For a full-size note:

`rawEarned += customNoteScore`

`rawMax += 100`

For a chain link:

`rawEarned += 20 if hit else 0`

`rawMax += 20`

Ignored objects add nothing to either side.

## Combo-weighted level accuracy

Level accuracy uses Beat Saber's combo multiplier values stored on each scoring element:

`levelEarned += objectScore * actualMultiplier`

`levelMax += objectMax * maxPossibleMultiplier`

`levelAccuracy = levelEarned / levelMax * 100`

This means a perfect 100-point full note at x8 contributes 800, while a perfect 20-point chain link at x8 contributes 160.

## Traversal diagnostic policy

During headset validation, an unobserved traversal interval is treated as a measurement failure, not proof of a slow cut. The full-size note receives neutral speed credit, but `speedObserved=false`; the Speed row omits that note from its denominator and the Quest log increments `traversalMissingCount`.

After live validation confirms stable traversal, this policy can be changed to score unknown traversal as zero or to reject custom scoring for that note.


## Built-in Beat Saber score override (v0.11)

CutAccuracy now patches Beat Saber's stored/built-in score numerator and max-score denominator so the
built-in result and select-screen percentage use the CutAccuracy LEVEL ACC
ratio. The mod does **not** write raw custom points into Beat Saber's vanilla
115-point denominator. Instead, after each tracked scoring object it computes:

```text
builtinScore = round(CutAccuracy_LEVEL_EARNED)
builtinMax   = round(CutAccuracy_LEVEL_MAX)
```

So a 95% CutAccuracy level with custom totals of 9500 / 10000 stores score
9500 and max 10000, producing 95.00%, not 9500 / 11500 = 82.61%.

This intentionally changes the built-in saved/local score path. Use it as an
offline/custom-scoring mod unless leaderboard behavior has been checked.
