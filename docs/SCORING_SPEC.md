# CutAccuracy v0.12.1 scoring spec

## Object policy

CutAccuracy counts scoreable cut objects only. Hazards and unhittable/non-score objects are ignored for both `RAW ACC` and `LEVEL ACC`.

| Beat Saber scoring object | Custom max | Mechanism |
|---|---:|---|
| Normal arrow note | 100 | blended swing/accuracy model |
| Normal dot note | 100 | same model, split axis from saber travel |
| ArcHead / ArcTail / supported combined full-size types | 100 | blended swing/accuracy model |
| ChainHead / supported combined chain-head types | 100 | blended swing/accuracy model |
| ChainLink / ChainLinkArcHead | 20 | fixed 20 on hit, 0 on miss/bad cut |
| NoScore / Ignore | 0 | excluded |
| Bombs/walls/hazards | 0 | ignored for accuracy |
| Unknown/unhittable | 0 | ignored for accuracy |

## Two /100 endpoint models

Each full-size note computes two complete scores out of 100, then blends them with the user setting.

### Swing-angle endpoint

Swing angle follows the standard 70/30 point split with fixed angle targets:

- Before swing: 70 points at 100 degrees.
- After swing: 30 points at 60 degrees.

Both are linear and clamp at full credit:

`before = 70 * clamp(beforeDegrees / 100, 0, 1)`

`after = 30 * clamp(afterDegrees / 60, 0, 1)`

`swingScore = before + after`

### Note-accuracy endpoint

The note is split by its cut direction into upper/lower halves, then each half is split again through note depth. This yields four independent mini-notes. Each mini-note is worth 25 points.

For each mini-note:

`miniScore = 25 * clamp((smallerVolumeRatio / 0.5), 0, 1)`

So 50/50 scores 25, 60/40 scores 20, and 100/0 scores 0.

`noteAccuracyScore = mini1 + mini2 + mini3 + mini4`

The HUD shows Upper and Lower as the averages of their two depth mini-notes, but all four 25-point mini-notes contribute independently to `noteAccuracyScore`.

## Scoring-style slider and presets

The setting is a whole-percent slider from 0 to 100, where the value is the note-accuracy weight.

`finalScore = swingScore * (1 - accuracyWeight) + noteAccuracyScore * accuracyWeight`

The only named presets are:

- Classic Feel: 0% note accuracy / 100% swing angle.
- Standard Beat Saber: 13% note accuracy / 87% swing angle.
- Precision Mode: 100% note accuracy / 0% swing angle.

The slider may still be placed at any whole percentage between these presets.

## Chain links

Chain links intentionally do not use the cube model. They remain fixed objects:

`hit = 20 / 20`

`miss or bad cut = 0 / 20`

They affect raw/level denominators but do not populate Upper, Lower, Before, or After HUD rows.

## Raw and combo-weighted level accuracy

`rawAccuracy = rawEarned / rawMax * 100`

`levelEarned += objectScore * actualMultiplier`

`levelMax += objectMax * maxPossibleMultiplier`

`levelAccuracy = levelEarned / levelMax * 100`

## Built-in Beat Saber score override

The blended full-note result is already a complete /100 score. CutAccuracy stores the rounded custom result in one 100-point Beat Saber score bucket, keeping the built-in denominator exact for every slider percentage.

The level numerator and max denominator are also rewritten into CutAccuracy's score space so a custom 9500 / 10000 remains 95.00%, rather than being interpreted against Beat Saber's normal 115-point note denominator.
