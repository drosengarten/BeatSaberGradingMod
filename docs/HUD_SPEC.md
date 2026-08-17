# CutAccuracy v0.12.1 HUD spec

The persistent panel remains attached above the stock Combo UI.

Normal layout:

```text
LEVEL ACC   93.72%
 RAW ACC    96.18%

          L       R
Upper    96.4    95.9
Lower    95.8    96.1
Before   97.3    96.8
After    94.9    95.4
```

There are exactly four metric rows per saber:

- Upper: average quality of the two upper depth mini-notes.
- Lower: average quality of the two lower depth mini-notes.
- Before: progress toward the 100-degree before-swing target.
- After: progress toward the 60-degree after-swing target.

If a particular saber has no valid sample for a metric, that cell displays `-` rather than `0` or `--`.

There is no speed row. Traversal speed does not contribute to score or HUD output.

Component rows use only full-size cube-model samples. Chain links still contribute to `LEVEL ACC` and `RAW ACC` as fixed 20/0 objects, but do not enter any of the four component-row denominators.
