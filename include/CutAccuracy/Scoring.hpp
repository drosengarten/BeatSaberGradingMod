#pragma once

#include "CutAccuracy/Geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace CutAccuracy {

enum class ScoreObjectKind { Excluded, FullNote, ChainLink };

struct ScoreObjectRule {
    ScoreObjectKind kind{ScoreObjectKind::Excluded};
    double maxScore{0.0};
    bool usesCubeModel{false};
    bool fixedHitScore{false};
    const char* name{"Excluded"};
};

inline int roundedNonNegativeToInt(double value) {
    if (value <= 0.0) return 0;
    const auto rounded = static_cast<long long>(std::llround(value));
    if (rounded <= 0) return 0;
    if (rounded > static_cast<long long>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(rounded);
}

inline int customInternalMaxScore(double customLevelMax) {
    return roundedNonNegativeToInt(customLevelMax);
}

inline int customInternalScoreFromCustomLevel(double customLevelEarned, double customLevelMax) {
    const int customMax = customInternalMaxScore(customLevelMax);
    if (customMax <= 0) return 0;
    const int customScore = roundedNonNegativeToInt(customLevelEarned);
    return std::clamp(customScore, 0, customMax);
}

inline int vanillaCompatibleScoreFromCustomLevel(double customLevelEarned, double customLevelMax, int vanillaMaxScore) {
    if (customLevelMax <= 0.0 || vanillaMaxScore <= 0) return 0;
    const double ratio = std::clamp(customLevelEarned / customLevelMax, 0.0, 1.0);
    const double scaled = ratio * static_cast<double>(vanillaMaxScore);
    const auto rounded = static_cast<long long>(std::llround(scaled));
    if (rounded <= 0) return 0;
    if (rounded >= vanillaMaxScore) return vanillaMaxScore;
    if (rounded > static_cast<long long>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(rounded);
}

inline ScoreObjectRule scoreObjectRuleForScoringType(int scoringType) {
    switch (scoringType) {
        case 1: return {ScoreObjectKind::FullNote, 100.0, true, false, "Normal"};
        case 2: return {ScoreObjectKind::FullNote, 100.0, true, false, "ArcHead"};
        case 3: return {ScoreObjectKind::FullNote, 100.0, true, false, "ArcTail"};
        case 4: return {ScoreObjectKind::FullNote, 100.0, true, false, "ChainHead"};
        case 6: return {ScoreObjectKind::FullNote, 100.0, true, false, "ArcHeadArcTail"};
        case 7: return {ScoreObjectKind::FullNote, 100.0, true, false, "ChainHeadArcTail"};
        case 9: return {ScoreObjectKind::FullNote, 100.0, true, false, "ChainHeadArcHead"};
        case 10: return {ScoreObjectKind::FullNote, 100.0, true, false, "ChainHeadArcHeadArcTail"};
        case 5: return {ScoreObjectKind::ChainLink, 20.0, false, true, "ChainLink"};
        case 8: return {ScoreObjectKind::ChainLink, 20.0, false, true, "ChainLinkArcHead"};
        case -1: return {ScoreObjectKind::Excluded, 0.0, false, false, "Ignore"};
        case 0: return {ScoreObjectKind::Excluded, 0.0, false, false, "NoScore"};
        default: return {ScoreObjectKind::Excluded, 0.0, false, false, "Unknown"};
    }
}

// Fixed scoring model. Swing uses Beat Saber's 100-degree before / 60-degree after
// targets. Note accuracy is four independent 25-point mini-notes: the two depth
// halves of the upper mini-note and the two depth halves of the lower mini-note.
struct ScoreWeights {
    double miniNoteMax{25.0};
    double beforeSwingMax{70.0};
    double afterSwingMax{30.0};
    double beforeSwingFullAngleDeg{100.0};
    double afterSwingFullAngleDeg{60.0};
};

inline int clampAccuracyWeightPercent(int value) {
    return std::clamp(value, 0, 100);
}

inline double smallerRatio(const MiniNoteVolumes& v) {
    if (v.total <= 1e-12) return 0.0;
    return std::min(v.positiveSide, v.negativeSide) / v.total;
}

// Linear volume accuracy: 50/50 = 25, 60/40 = 20, 100/0 = 0.
inline double miniNoteScoreFromSmallerRatio(double smallerRatioValue, const ScoreWeights& w = {}) {
    const double ratio = std::clamp(smallerRatioValue, 0.0, 0.5);
    return w.miniNoteMax * (ratio / 0.5);
}

// Upper/lower HUD values are the average of their two depth mini-notes. Multiplying
// the two upper/lower averages by two later exactly reconstructs all four 25-point
// mini-note contributions for the 100-point note-accuracy endpoint.
inline double miniNoteScoreFromDepthSplitRatios(double negativeDepthRatio, double positiveDepthRatio, const ScoreWeights& w = {}) {
    return 0.5 * miniNoteScoreFromSmallerRatio(negativeDepthRatio, w)
        + 0.5 * miniNoteScoreFromSmallerRatio(positiveDepthRatio, w);
}

inline double miniNoteScoreFromDepthSplitVolumes(const DepthSplitMiniNoteVolumes& volumes, const ScoreWeights& w = {}) {
    return miniNoteScoreFromDepthSplitRatios(
        smallerRatio(volumes.negativeDepth),
        smallerRatio(volumes.positiveDepth),
        w
    );
}

inline double swingScore(double degrees, double maxPoints, double fullAngleDeg) {
    if (fullAngleDeg <= 0.0 || maxPoints <= 0.0) return 0.0;
    const double normalized = std::clamp(degrees / fullAngleDeg, 0.0, 1.0);
    return maxPoints * normalized;
}

inline double beforeSwingScore(double degrees, const ScoreWeights& w = {}) {
    return swingScore(degrees, w.beforeSwingMax, w.beforeSwingFullAngleDeg);
}

inline double afterSwingScore(double degrees, const ScoreWeights& w = {}) {
    return swingScore(degrees, w.afterSwingMax, w.afterSwingFullAngleDeg);
}

inline double blendedScore(double swingAngleScore, double noteAccuracyScore, int accuracyWeightPercent) {
    const double accuracyWeight = static_cast<double>(clampAccuracyWeightPercent(accuracyWeightPercent)) / 100.0;
    const double swingWeight = 1.0 - accuracyWeight;
    return std::clamp(swingWeight * swingAngleScore + accuracyWeight * noteAccuracyScore, 0.0, 100.0);
}

struct NoteComponents {
    // firstMini/secondMini are upper/lower HUD summaries, each 0..25. Each one is
    // the mean of two independent depth mini-note scores.
    double firstMini{0.0};
    double secondMini{0.0};
    // Swing contributions are already in the standard 70/30 point split.
    double beforeSwing{0.0};
    double afterSwing{0.0};
    int accuracyWeightPercent{13};

    double noteAccuracyScore() const {
        return std::clamp(2.0 * (firstMini + secondMini), 0.0, 100.0);
    }

    double swingAngleScore() const {
        return std::clamp(beforeSwing + afterSwing, 0.0, 100.0);
    }

    double total() const {
        return blendedScore(swingAngleScore(), noteAccuracyScore(), accuracyWeightPercent);
    }
};

struct BeatSaberCutScoreParts {
    int centerDistance{0};
    int before{0};
    int after{0};
    int fixed{0};
};

inline int roundedClampedToInt(double value, int minValue, int maxValue) {
    return std::clamp(roundedNonNegativeToInt(value), minValue, maxValue);
}

inline BeatSaberCutScoreParts beatSaberCutScoreParts(const NoteComponents& c, const ScoreWeights& w = {}) {
    // Keep Beat Saber's native swing-rating path alive by preserving the standard
    // 70/30 before/after score buckets. The custom blended result is still an exact
    // integer /100 score; we simply encode that integer across the two buckets.
    const int total = roundedClampedToInt(c.total(), 0, 100);
    const int beforeMax = roundedClampedToInt(w.beforeSwingMax, 0, 100);
    const int afterMax = roundedClampedToInt(w.afterSwingMax, 0, 100 - beforeMax);
    const int before = std::min(total, beforeMax);
    const int after = std::min(total - before, afterMax);
    return {0, before, after, 0};
}

struct RawMeasurements {
    double firstMiniSmallerRatio{0.0};
    double secondMiniSmallerRatio{0.0};
    double beforeSwingDeg{0.0};
    double afterSwingDeg{0.0};
};

inline NoteComponents score(const RawMeasurements& m, int accuracyWeightPercent = 13, const ScoreWeights& w = {}) {
    return {
        miniNoteScoreFromSmallerRatio(m.firstMiniSmallerRatio, w),
        miniNoteScoreFromSmallerRatio(m.secondMiniSmallerRatio, w),
        beforeSwingScore(m.beforeSwingDeg, w),
        afterSwingScore(m.afterSwingDeg, w),
        clampAccuracyWeightPercent(accuracyWeightPercent)
    };
}

} // namespace CutAccuracy
