#pragma once

#include "CutAccuracy/Stats.hpp"

#include <string>

namespace CutAccuracy {

struct HudPresentation {
    double combinedAccuracyPct{0.0};
    std::string heading;
    std::string table;
};

struct AccuracyRgb {
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
};

// Pure formatting layer used by the Quest HUD. Keeping this free of Unity/BSML
// makes layout text deterministic and host-testable.
HudPresentation buildHudPresentation(const SessionStats& stats);

// Compact integer score used to replace the normal flying cut score.
std::string formatPerNoteScore(const NoteComponents& components);
std::string formatFixedScore(double score, double maxScore);

int accuracyBand5(double accuracyPct);
const char* accuracyBandHex(double accuracyPct);
AccuracyRgb accuracyBandRgb(double accuracyPct);

} // namespace CutAccuracy
