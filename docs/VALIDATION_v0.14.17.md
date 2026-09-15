# Squore Customiser 0.14.17 verification

## Package identity

- Mod name: Squore Customiser
- Mod ID: `cutaccuracy`
- Author: Daniel Rosengarten
- Version: `0.14.17`
- Game target: Beat Saber `1.40.8_7379`
- Modloader: Scotland2
- QMOD: `SquoreCustomiser-0.14.17.qmod`
- `libcutaccuracy.so` SHA-256: `5664f169dbc943fe2af94e9b1bfde14ef21b46576058c5cdd35022c30e2d5d7f`
- `SquoreCustomiser-0.14.17.qmod` SHA-256: `14668ba1fdb00ac7aeec52e1d31df2d4b9e0c53b4fb7b6e9316687fef41424ac`
- `cover.png` SHA-256: `0d8555d577087cbda0a52d37a7f1d7f652216077809444a2127f42aadb49e4d4`

## Leaderboard submission policy

Verified against `extern/includes/metacore/shared/game.hpp`: `MetaCore::Game::SetScoreSubmission(mod, enable)` enables submission for this mod only, and global submission remains disabled if any mod keeps a disabler registered.

Squore Customiser uses one tested rule:

- Off / vanilla mode: `scoreSubmissionAllowedForMode(ScoringMode::Off) == true`, so Squore Customiser releases its MetaCore submission block.
- Simple mode: `scoreSubmissionAllowedForMode(ScoringMode::Simple) == false`, so Squore Customiser blocks external score submission.
- Advanced mode: `scoreSubmissionAllowedForMode(ScoringMode::Advanced) == false`, so Squore Customiser blocks external score submission.

`UpdateScoreSubmissionPolicy()` applies that rule through `MetaCore::Game::SetScoreSubmission(MOD_ID, CutAccuracy::scoreSubmissionAllowedForMode(CurrentScoringMode()))`. The in-game mode selector calls this immediately when the player changes modes, and hook installation calls it at load time.

BeatLeader was present on the test headset. I did not find a ScoreSaber loader library in `/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods`; only Qounters artwork referenced ScoreSaber. The MetaCore policy is the compatibility layer used for third-party leaderboard mods that honor it.

## Scoring fixes and checks

- Added tests for the submission policy so Simple/Advanced block and Off/vanilla allows.
- Added per-cut carrier snapshots so a shared Beat Saber score definition cannot leak one note type's max score into another note type's finish calculation. This matters for Dot vs Directional notes because both are scoring type `1`.
- Kept custom carrier maxima aligned with profile maxima, including max-score `100`, so Beat Saber's level percentage does not divide custom scores by stale native maxima like `115`.
- Confirmed component sliders are bounded by the remaining 100% budget.
- Hardened bad-cut and miss sliders so stale/out-of-range delivered values are clamped before saving.
- Confirmed typed max-score fields reject non-digits and restore the current numeric value.
- Confirmed non-finite scoring values fail closed to zero and arbitrary profile edits keep scores within `0..maxScore` through fuzz tests.

## Commands run

- `./scripts/test-host.sh` — passed.
- `cmake --build build -j 4` — passed.
- `git diff --check` — passed.
- `tools/qpm/qpm qmod zip` — passed.

## MBF fresh install check

Before the fresh-install check, the previous Squore Customiser package folder and active loader copy were removed from the headset:

- `/sdcard/ModData/com.beatgames.beatsaber/Packages/1.40.8_7379/cutaccuracy_v0.14.17`
- `/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/libcutaccuracy.so`

Then `SquoreCustomiser-0.14.17.qmod` was pushed to `/data/local/tmp/mbf/uploads/SquoreCustomiser-0.14.17.qmod` and imported through `mbf-agent`.

The final import log includes:

- `Early load of new mod, ID cutaccuracy, version: 0.14.17, author: Daniel Rosengarten`
- `Extracting cutaccuracy v0.14.17`
- MBF imported the mod as `cutaccuracy` with display name `Squore Customiser`.
- `Extract path: "/sdcard/ModData/com.beatgames.beatsaber/Packages/1.40.8_7379/cutaccuracy_v0.14.17"`

The final enable log includes:

- `Installing cutaccuracy v0.14.17`
- `Copying "libcutaccuracy.so" to "/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/libcutaccuracy.so"`
- `Installed cutaccuracy`
- `failures: null`

Installed headset files:

- Active loader library: `/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/libcutaccuracy.so`
- Package library: `/sdcard/ModData/com.beatgames.beatsaber/Packages/1.40.8_7379/cutaccuracy_v0.14.17/libcutaccuracy.so`
- Package manifest: `/sdcard/ModData/com.beatgames.beatsaber/Packages/1.40.8_7379/cutaccuracy_v0.14.17/mod.json`

The headset copies of the active loader library and package library both matched the local library hash above. The headset package manifest reported display name `Squore Customiser`, ID `cutaccuracy`, version `0.14.17`, author `Daniel Rosengarten`, and `coverImage: cover.png`.

Raw MBF logs are stored in:

- `artifacts/v0.14.17/mbf-squore-import.json`
- `artifacts/v0.14.17/mbf-squore-enable.json`
- `artifacts/v0.14.17/mod-squore-installed.json`
- `artifacts/v0.14.17/sha256-squore-installed.txt`

## Runtime limitation

ADB launch was stopped by Meta's controller-required prompt: `com.oculus.vrshell/.systemdialog.launchcheck.LaunchCheckControllerRequiredDialogActivity`. That blocks final Unity log confirmation until the prompt is cleared in-headset and Beat Saber opens normally. The package-level fresh-install proof is complete.
