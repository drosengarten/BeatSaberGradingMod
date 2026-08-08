# CutAccuracy v0.11 HUD spec

The persistent panel is attached to the stock Combo UI hierarchy and positioned above the live Combo text's top edge.

Normal layout:

```text
LEVEL ACC   93.72%
 RAW ACC    96.18%

          L       R
Upper    96.4    95.9
Lower    95.8    96.1
Before   97.3    96.8
After    94.9    95.4
Speed    99.2    99.0

        COMBO
         248
```

The panel itself does not replace Beat Saber's stock Combo, rank, multiplier, score text, or lane objects. In v0.11, however, the underlying built-in score value is deliberately overridden so the stock result/select-screen percentage follows CutAccuracy LEVEL ACC.

`HudTuning` exposes:

- `xOffset`
- `yOffset`
- `scale`
- `width`
- `height`
- `fontSize`

These values are grouped in `include/Quest/HudModel.hpp` and `src/Quest/HudModel.cpp` so headset tuning is not mixed into scoring logic.


Component rows use only full-size cube-model samples. Chain links contribute to LEVEL ACC and RAW ACC as fixed 20/0 objects, but they do not enter the Upper, Lower, Before, After, or Speed row denominators. Hazards and unscoreable objects do not change any displayed accuracy value.
