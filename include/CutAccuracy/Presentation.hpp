#pragma once
#include "CutAccuracy/Stats.hpp"
#include <string>
namespace CutAccuracy {
struct HudPresentation { double combinedAccuracyPct{0}; std::string heading; std::string table; };
struct AccuracyRgb { float r{1},g{1},b{1}; };
HudPresentation buildHudPresentation(const SessionStats& stats,ScoringMode mode,AccuracyMethod simpleMethod=AccuracyMethod::Precise,bool showPreciseInternals=false);
std::string formatPerNoteScore(const ScoredNote& scored);
std::string formatFixedScore(double score,double maxScore);
int accuracyBand5(double accuracyPct); const char* accuracyBandHex(double accuracyPct); AccuracyRgb accuracyBandRgb(double accuracyPct);
}
