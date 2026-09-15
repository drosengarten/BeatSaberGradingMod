# Squore Customiser v0.14.17 scoring specification

## Modes

### Off
Squore Customiser does not alter score definitions, does not attach the custom scoring completion delegate, and does not install its HUD for the play session.

### Simple
Simple exposes one shared full-note model and one chain-link maximum:
- Accuracy method: Beat Saber Center or Precise
- Full-note maximum
- Chain-link maximum
- Accuracy / Before / After weights (each capped by the remaining available percentage)
- Before and After full-credit angles
- Precise Upper-pair / Lower-pair split

### Advanced
Every scoreable object kind gets a separate `ScoringProfile`:
- Directional Note
- Dot Note
- Arc Head
- Arc Tail
- Chain Head
- Chain Link
- Arc Head + Arc Tail
- Chain Head + Arc Tail
- Chain Head + Arc Head
- Chain Head + Arc Head + Arc Tail
- Chain Link + Arc Head

Each profile independently owns its maximum, Flat weight, enabled components, component weights, swing full-credit angles, Precise Upper/Lower pair split, bad-cut rule, and miss rule.


## Default Advanced: Beat Saber-like rounded profiles

Fresh installs start in **Advanced**. The initial profiles use Beat Saber 1.40.8 scoring-type maxima and map the native point shares to the nearest whole percent. Because enabling Squore Customiser means custom scoring is active, these are intentionally easy-to-read approximations rather than fractional replicas.

| Profile | Max | Flat | Precise | Center | Before | After |
|---|---:|---:|---:|---:|---:|---:|
| Directional Note | 115 | 0% | Off | 13% | 61% @ 100° | 26% @ 60° |
| Dot Note | 115 | 0% | Off | 13% | 61% @ 100° | 26% @ 60° |
| Arc Head | 115 | 26% | Off | 13% | 61% @ 100° | Off |
| Arc Tail | 115 | 61% | Off | 13% | Off | 26% @ 60° |
| Chain Head | 85 | 0% | Off | 18% | 82% @ 100° | Off |
| Chain Link | 20 | 100% | Off | Off | Off | Off |
| Arc Head + Arc Tail | 115 | 87% | Off | 13% | Off | Off |
| Chain Head + Arc Tail | 115 | 87% | Off | 13% | Off | Off |
| Chain Head + Arc Head | 115 | 26% | Off | 13% | 61% @ 100° | Off |
| Chain Head + Arc Head + Arc Tail | 115 | 87% | Off | 13% | Off | Off |
| Chain Link + Arc Head | 20 | 100% | Off | Off | Off | Off |

Every row totals 100%. Flat is not an absolute point amount: it is the guaranteed valid-cut share of that profile's `Max Score`. For example, Arc Head's 26% Flat share contributes about 29.9 points when Max Score is 115. Precise defaults to OFF. Bad Cut and Miss default to zero.

## Four-mini-note Precise Accuracy

The implementation does **not** reduce Precise Accuracy to two mini-notes.

For a signed note-direction axis `D`, the note is first divided through its center into:
- **Upper**: `dot(D, p) >= 0`, the half toward the note's cut direction.
- **Lower**: `dot(D, p) <= 0`, the opposite half.

Each of those halves is then divided by note-local depth (`z=0`). The resulting four score regions are:

1. Upper / negative depth
2. Upper / positive depth
3. Lower / negative depth
4. Lower / positive depth

Each region is cut by the saber plane and independently receives:

`MiniQuality = 2 * min(Vpositive, Vnegative) / (Vpositive + Vnegative)`

clamped to `[0,1]`.

The two Upper values are averaged and the two Lower values are averaged:

`UpperQuality = (U1 + U2) / 2`

`LowerQuality = (L1 + L2) / 2`

The user-facing Precise split controls only the weighting between those two **pairs**:

`PreciseQuality = UpperQuality * UpperWeight + LowerQuality * (1 - UpperWeight)`

Thus all four geometric measurements remain independent even when Upper/Lower are set to 50/50.

### Direction assignment
Mapped directional notes use the full signed arrow direction, including cut-direction angle offset. Opposite arrows therefore reverse the physical Upper/Lower halves. Dot notes have no mapped arrow, so their signed axis is the actual saber travel direction at the cut.

## Valid-cut score

For one profile:

All scoring-component weights — **Flat, Precise, Center, Before, After** — share one direct percentage budget of MaxScore. Enabled weights may sum to **at most 100%**. The UI caps each weight at `100% - sum(other enabled weights)` and shows any remainder as Remaining. A profile with remaining weight intentionally cannot earn that unused fraction of its maximum score.

`Score = MaxScore * (`
`  FlatWeight +`
`  PreciseQuality * PreciseWeight +`
`  CenterQuality * CenterWeight +`
`  BeforeQuality * BeforeWeight +`
`  AfterQuality * AfterWeight`
`)`

Disabled components contribute zero and hold zero weight.

`BeforeQuality = clamp(actualBeforeDegrees / beforeFullCreditDegrees, 0, 1)`

`AfterQuality = clamp(actualAfterDegrees / afterFullCreditDegrees, 0, 1)`

Center Accuracy uses Beat Saber's native center-distance result normalized from its 15-point measurement channel.

## Failure rules
Bad cuts and misses are independently configurable per Advanced profile as:
- Zero
- Fixed points
- Percent of that profile's maximum

A miss does not fabricate geometry or swing-component values.

## Chain links
Chain links use the same generic profile engine. The default Simple-derived profile is `Max=20`, `Flat=100%`, all measured components off, which naturally reproduces binary `20/0` behavior. Advanced users may change that profile independently.

## Native Beat Saber score carrier
For custom scoring, native cut-score fields remain available to Beat Saber's lifecycle/swing machinery. The final custom cut percentage is encoded into the native 70/30 before/after carrier, while the custom absolute score and custom maximum are tracked independently and synchronized into `ScoreController`.

### Whole-percent Advanced weights
Advanced component weights — including Flat — are integer percentages only (0–100%). Each weight slider moves in 1% steps and its live maximum is `100 - sum(other enabled weights)`. Changing one weight never redistributes the others. A total below 100% is shown as Remaining. Beat Saber-like defaults are rounded to whole percentages (for example Normal/Dot = Center 13%, Before 61%, After 26%; Arc Head = Flat 26%, Center 13%, Before 61%).


## Score submission safety

When **Simple** or **Advanced** custom scoring is selected, Squore Customiser registers itself as a MetaCore score-submission disabler. **Off** removes that disabler. This prevents custom maxima (including values above vanilla) from being submitted to third-party leaderboards that honor MetaCore submission state.

If required scoring hooks are unavailable, custom scoring fails closed: Beat Saber scoring is left untouched, while submission remains disabled until the player selects Off.
