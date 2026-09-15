# Cut Accuracy 0.14.17

Cut Accuracy is a Quest Beat Saber mod for Beat Saber `1.40.8_7379` using the Scotland2 modloader. It lets players choose vanilla scoring, a simple shared custom profile, or advanced per-note-type scoring profiles.

## Release package

The release qmod is built as `CutAccuracy-0.14.17.qmod` and declares the mod ID `cutaccuracy`, author `Daniel Rosengarten`, and the package target `com.beatgames.beatsaber` / `1.40.8_7379`.

The qmod includes `libcutaccuracy.so` as a late-loaded mod library and declares MBF-resolvable dependencies for beatsaber-hook, custom-types, paper2_scotland2, BSML, and MetaCore.

## Player settings

Cut Accuracy has three scoring modes:

- **Off** leaves Beat Saber scoring unchanged and releases Cut Accuracy's leaderboard-submission block.
- **Simple** applies one full-note profile and one chain-link maximum. It exposes the main scoring pieces players are most likely to adjust: max score, accuracy method, accuracy/before/after weights, swing angles, and precise upper/lower balance.
- **Advanced** gives every scoreable Beat Saber note type its own profile. Each profile can set max score, accuracy method, flat score share, center accuracy, swing weighting, precise weighting, bad-cut behavior, and miss behavior.

The settings UI is organized around mod-wide actions at the top, then Simple or Advanced controls depending on the selected mode. Advanced profile labels use full readable names such as `Directional`, `Dot`, `Arc Head`, and `Chain Link`, with shortened combination names only where the native note type is a combination.

## Scoring behavior

Fresh installs default to Advanced mode with Beat Saber-like rounded profiles. The defaults preserve the native scoring-type maximums and use whole-percent component weights.

Every custom profile computes a score from a direct percentage budget of the profile's max score:

```text
Score = MaxScore * (Flat + Precise + Center + Before + After contributions)
```

Flat is a percentage of the profile maximum. It is not an independent point value. For example, a 10% Flat value on a 100-point note contributes 10 points.

Precise accuracy still uses four mini-note regions. The cube is split into upper/lower halves and each half is split by depth, then the two upper regions and two lower regions are weighted by the player's precise upper/lower setting.

## Leaderboard submission safety

When Simple or Advanced custom scoring is active, Cut Accuracy registers a MetaCore score-submission block so compatible leaderboard mods do not submit altered scores. Off mode releases that block and allows vanilla scoring submissions again.

## Building

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
tools/qpm/qpm qmod zip
```

Host-side validation can be run with:

```bash
./scripts/test-host.sh
```

## 0.14.17 validation summary

- Host scoring, geometry, presentation, and submission-policy tests pass.
- Quest build completes with CMake/QPM.
- `tools/qpm/qpm qmod zip` creates a valid qmod.
- MBF fresh import and enable were tested on a headset after removing the previous installed Cut Accuracy package and active loader copy.
- The installed headset library and qmod package library matched the local build hash.

Current verified hashes:

- `libcutaccuracy.so`: `16ae6c604d490d94b729cec5035ec75b7d6a5963c79797212da41fb10645cdd4`
- `CutAccuracy-0.14.17.qmod`: `b1b60dac722f68e3b793f6aa9e951275642a6e5ed9f68bf6d8fb7b808886bc17`

Detailed release verification is kept in `docs/VALIDATION_v0.14.17.md`.
