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
    // Option B true-internal mode: Beat Saber should no longer use the 115-point
    // normal-note max for CutAccuracy levels. The built-in max score itself is
    // rewritten to the rounded CutAccuracy max.
    return roundedNonNegativeToInt(customLevelMax);
}

inline int customInternalScoreFromCustomLevel(double customLevelEarned, double customLevelMax) {
    const int customMax = customInternalMaxScore(customLevelMax);
    if (customMax <= 0) return 0;
    const int customScore = roundedNonNegativeToInt(customLevelEarned);
    return std::clamp(customScore, 0, customMax);
}

inline int vanillaCompatibleScoreFromCustomLevel(double customLevelEarned, double customLevelMax, int vanillaMaxScore) {
    // Legacy fallback/projection helper kept for tests and emergency debugging.
    // v0.11 no longer relies on this as the primary route; it patches Beat
    // Saber's score model/max denominator to CutAccuracy's custom max instead.
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
    // Beat Saber NoteData.ScoringType values for 1.40.8:
    // -1 Ignore, 0 NoScore, 1 Normal, 2 ArcHead, 3 ArcTail,
    // 4 ChainHead, 5 ChainLink, 6 ArcHeadArcTail, 7 ChainHeadArcTail,
    // 8 ChainLinkArcHead. Values 9 and 10 are present in some parser
    // references as combined chain-head/arc forms, so support them defensively.
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


struct ScoreWeights {
    double miniNoteMax{25.0};
    double beforeSwingMax{20.0};
    double afterSwingMax{20.0};
    double speedMax{10.0};
    double swingFullAngleDeg{60.0};
    double speedFullTimeSeconds{0.100};
};

struct NoteComponents {
    double firstMini{0.0};
    double secondMini{0.0};
    double beforeSwing{0.0};
    double afterSwing{0.0};
    double speed{0.0};
    bool speedObserved{true};

    double total() const {
        return firstMini + secondMini + beforeSwing + afterSwing + speed;
    }
};

struct RawMeasurements {
    double firstMiniSmallerRatio{0.0}; // 0..0.5
    double secondMiniSmallerRatio{0.0}; // 0..0.5
    double beforeSwingDeg{0.0};
    double afterSwingDeg{0.0};
    double traversalSeconds{0.0};
};

inline double miniNoteScoreFromSmallerRatio(double smallerRatio, const ScoreWeights& w = {}) {
    // 50/50 => 0.5 * 50 = 25.
    const double ratio = std::clamp(smallerRatio, 0.0, 0.5);
    return ratio * (w.miniNoteMax / 0.5);
}

inline double swingScore(double degrees, double maxPoints, const ScoreWeights& w = {}) {
    const double frac = std::clamp(degrees / w.swingFullAngleDeg, 0.0, 1.0);
    return maxPoints * frac;
}

inline double speedScore(double traversalSeconds, const ScoreWeights& w = {}) {
    if (traversalSeconds <= 0.0) return 0.0;
    // Full points at 100 ms or faster; inverse falloff beyond that.
    return w.speedMax * std::min(1.0, w.speedFullTimeSeconds / traversalSeconds);
}

inline NoteComponents score(const RawMeasurements& m, const ScoreWeights& w = {}) {
    return {
        miniNoteScoreFromSmallerRatio(m.firstMiniSmallerRatio, w),
        miniNoteScoreFromSmallerRatio(m.secondMiniSmallerRatio, w),
        swingScore(m.beforeSwingDeg, w.beforeSwingMax, w),
        swingScore(m.afterSwingDeg, w.afterSwingMax, w),
        speedScore(m.traversalSeconds, w),
        true
    };
}

inline NoteComponents scoreWithUnknownSpeed(const RawMeasurements& m, const ScoreWeights& w = {}) {
    auto c = score(m, w);
    // During headset validation a missing traversal interval is a measurement failure,
    // not proof of a slow cut. Give neutral/full speed credit, but mark the component
    // as unobserved so the HUD and logs show that speed data is not yet trustworthy.
    c.speed = w.speedMax;
    c.speedObserved = false;
    return c;
}

inline double smallerRatio(const MiniNoteVolumes& v) {
    if (v.total <= 1e-12) return 0.0;
    return std::min(v.positiveSide, v.negativeSide) / v.total;
}

} // namespace CutAccuracy
