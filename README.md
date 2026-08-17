# CutAccuracy v0.12.2

Quest standalone Beat Saber 1.40.8 source package for a custom /100 cut-scoring model.

## Full-size note scoring

Every full-size scoreable note computes two complete /100 endpoint scores:

- **Swing angle:** 70 points before the cut at 100 degrees, plus 30 points after the cut at 60 degrees.
- **Note accuracy:** four independent geometric mini-notes worth 25 points each.

A whole-percent slider blends those endpoint scores. The only named presets are:

- **Classic Feel:** 100% swing angle / 0% note accuracy.
- **Standard Beat Saber:** 87% swing angle / 13% note accuracy.
- **Precision Mode:** 0% swing angle / 100% note accuracy.

The slider remains continuous between those presets and snaps to the nearest whole percent. The settings screen shows `Swing angle x% / Note accuracy y%` below it.

## HUD

The HUD keeps `LEVEL ACC` and `RAW ACC`, plus exactly four per-saber rows:

- Upper
- Lower
- Before
- After

Missing metric data for a saber displays `-`.

## Custom flying-score text

Custom below-note text is optional. There is one editable phrase for each range: `0-9`, `10-19`, ... `90-99`, and `100`.

## Other object handling

- Full-size normal notes, supported arc endpoints, and chain heads use the /100 model.
- Chain links remain fixed 20 on hit / 0 on miss or bad cut.
- Bombs, walls, `NoScore`, `Ignore`, and unknown/unhittable objects do not affect custom accuracy denominators.

## Built-in score override

CutAccuracy rewrites Beat Saber's score/max-score path into the custom score space so the built-in percentage follows `LEVEL ACC` rather than treating custom notes as if they were still out of 115.
