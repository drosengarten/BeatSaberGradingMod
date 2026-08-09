#pragma once

#include "CutAccuracy/Geometry.hpp"
#include <algorithm>
#include <array>
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
    using QualityCurveAnchors = std::array<double, 11>;

    double miniNoteMax{25.0};
    double beforeSwingMax{20.0};
    double afterSwingMax{20.0};
    double speedMax{10.0};
    double swingFullAngleDeg{60.0};
    double speedFullTimeSeconds{0.150};
    QualityCurveAnchors qualityCurve{
        0.0, 0.07, 0.23, 0.43, 0.64, 0.81, 0.90, 0.95, 0.975, 0.99, 1.0
    };
};

enum class DifficultyProfile {
    Easy = 0,
    Normal = 1,
    Hard = 2,
    Expert = 3,
    ExpertPlus = 4
};

inline DifficultyProfile difficultyProfileFromIndex(int index) {
    switch (index) {
        case 0: return DifficultyProfile::Easy;
        case 2: return DifficultyProfile::Hard;
        case 3: return DifficultyProfile::Expert;
        case 4: return DifficultyProfile::ExpertPlus;
        case 1:
        default: return DifficultyProfile::Normal;
    }
}

inline const char* difficultyProfileName(DifficultyProfile profile) {
    switch (profile) {
        case DifficultyProfile::Easy: return "Easy";
        case DifficultyProfile::Hard: return "Hard";
        case DifficultyProfile::Expert: return "Expert";
        case DifficultyProfile::ExpertPlus: return "Expert+";
        case DifficultyProfile::Normal:
        default: return "Normal";
    }
}

inline constexpr ScoreWeights::QualityCurveAnchors kEasyQualityCurve{
    0.0, 0.10, 0.28, 0.50, 0.70, 0.86, 0.93, 0.97, 0.99, 0.996, 1.0
};

inline constexpr ScoreWeights::QualityCurveAnchors kNormalQualityCurve{
    0.0, 0.07, 0.23, 0.43, 0.64, 0.81, 0.90, 0.95, 0.975, 0.99, 1.0
};

inline constexpr ScoreWeights::QualityCurveAnchors kHardQualityCurve{
    0.0, 0.05, 0.18, 0.35, 0.55, 0.73, 0.84, 0.92, 0.96, 0.984, 1.0
};

inline constexpr ScoreWeights::QualityCurveAnchors kExpertQualityCurve{
    0.0, 0.03, 0.13, 0.28, 0.47, 0.66, 0.79, 0.89, 0.94, 0.976, 1.0
};

inline constexpr ScoreWeights::QualityCurveAnchors kExpertPlusQualityCurve{
    0.0, 0.02, 0.09, 0.21, 0.38, 0.58, 0.73, 0.85, 0.92, 0.968, 1.0
};

inline ScoreWeights scoreWeightsForDifficulty(DifficultyProfile profile) {
    switch (profile) {
        case DifficultyProfile::Easy:
            return {25.0, 20.0, 20.0, 10.0, 50.0, 0.200, kEasyQualityCurve};
        case DifficultyProfile::Hard:
            return {25.0, 20.0, 20.0, 10.0, 70.0, 0.125, kHardQualityCurve};
        case DifficultyProfile::Expert:
            return {25.0, 20.0, 20.0, 10.0, 80.0, 0.100, kExpertQualityCurve};
        case DifficultyProfile::ExpertPlus:
            return {25.0, 20.0, 20.0, 10.0, 90.0, 0.086, kExpertPlusQualityCurve};
        case DifficultyProfile::Normal:
        default:
            return {};
    }
}

inline ScoreWeights scoreWeightsForDifficultyIndex(int index) {
    return scoreWeightsForDifficulty(difficultyProfileFromIndex(index));
}

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
    const int maxCenterAndSpeed = roundedNonNegativeToInt((w.miniNoteMax * 2.0) + w.speedMax);
    const int maxBefore = roundedNonNegativeToInt(w.beforeSwingMax);
    const int maxAfter = roundedNonNegativeToInt(w.afterSwingMax);
    const int maxTotal = maxCenterAndSpeed + maxBefore + maxAfter;

    const int target = roundedClampedToInt(c.total(), 0, maxTotal);
    int before = roundedClampedToInt(c.beforeSwing, 0, std::min(maxBefore, target));
    int after = roundedClampedToInt(c.afterSwing, 0, std::min(maxAfter, target - before));
    int center = target - before - after;

    if (center > maxCenterAndSpeed) {
        const int overflow = center - maxCenterAndSpeed;
        center = maxCenterAndSpeed;
        const int beforeRoom = maxBefore - before;
        const int beforeFill = std::min(overflow, beforeRoom);
        before += beforeFill;
        after += std::min(overflow - beforeFill, maxAfter - after);
    }

    return {center, before, after, 0};
}

struct RawMeasurements {
    double firstMiniSmallerRatio{0.0}; // 0..0.5
    double secondMiniSmallerRatio{0.0}; // 0..0.5
    double beforeSwingDeg{0.0};
    double afterSwingDeg{0.0};
    double traversalSeconds{0.0};
};

inline double smallerRatio(const MiniNoteVolumes& v) {
    if (v.total <= 1e-12) return 0.0;
    return std::min(v.positiveSide, v.negativeSide) / v.total;
}

inline bool sameSign(double lhs, double rhs) {
    return (lhs > 0.0 && rhs > 0.0) || (lhs < 0.0 && rhs < 0.0);
}

inline double pchipEndpointSlope(double edgeDelta, double nextDelta) {
    double slope = (3.0 * edgeDelta - nextDelta) * 0.5;
    if (!sameSign(slope, edgeDelta)) return 0.0;
    if (!sameSign(edgeDelta, nextDelta) && std::abs(slope) > std::abs(3.0 * edgeDelta)) {
        return 3.0 * edgeDelta;
    }
    return slope;
}

inline double pchipSlope(const ScoreWeights::QualityCurveAnchors& anchors, std::size_t index) {
    constexpr double h = 1.0 / static_cast<double>(std::tuple_size_v<ScoreWeights::QualityCurveAnchors> - 1);
    constexpr std::size_t last = std::tuple_size_v<ScoreWeights::QualityCurveAnchors> - 1;
    auto delta = [&](std::size_t i) {
        return (anchors[i + 1] - anchors[i]) / h;
    };

    if (index == 0) return pchipEndpointSlope(delta(0), delta(1));
    if (index == last) return pchipEndpointSlope(delta(last - 1), delta(last - 2));

    const double previous = delta(index - 1);
    const double next = delta(index);
    if (!sameSign(previous, next)) return 0.0;
    return (2.0 * previous * next) / (previous + next);
}

inline double qualityCurveValue(double normalizedQuality, const ScoreWeights& w = {}) {
    constexpr std::size_t last = std::tuple_size_v<ScoreWeights::QualityCurveAnchors> - 1;
    constexpr double segmentWidth = 1.0 / static_cast<double>(last);

    const double q = std::clamp(normalizedQuality, 0.0, 1.0);
    if (q <= 0.0) return w.qualityCurve.front();
    if (q >= 1.0) return w.qualityCurve.back();

    const std::size_t segment = std::min(static_cast<std::size_t>(q / segmentWidth), last - 1);
    const double x0 = static_cast<double>(segment) * segmentWidth;
    const double t = (q - x0) / segmentWidth;
    const double t2 = t * t;
    const double t3 = t2 * t;

    const double y0 = w.qualityCurve[segment];
    const double y1 = w.qualityCurve[segment + 1];
    const double m0 = pchipSlope(w.qualityCurve, segment);
    const double m1 = pchipSlope(w.qualityCurve, segment + 1);

    const double h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const double h10 = t3 - 2.0 * t2 + t;
    const double h01 = -2.0 * t3 + 3.0 * t2;
    const double h11 = t3 - t2;
    return std::clamp(h00 * y0 + h10 * segmentWidth * m0 + h01 * y1 + h11 * segmentWidth * m1, 0.0, 1.0);
}

inline double miniNoteScoreFromSmallerRatio(double smallerRatio, const ScoreWeights& w = {}) {
    const double ratio = std::clamp(smallerRatio, 0.0, 0.5);
    return w.miniNoteMax * qualityCurveValue(ratio / 0.5, w);
}

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

inline double swingScore(double degrees, double maxPoints, const ScoreWeights& w = {}) {
    if (w.swingFullAngleDeg <= 0.0) return 0.0;
    return maxPoints * qualityCurveValue(degrees / w.swingFullAngleDeg, w);
}

inline double speedScore(double traversalSeconds, const ScoreWeights& w = {}) {
    if (traversalSeconds <= 0.0) return 0.0;
    return w.speedMax * qualityCurveValue(w.speedFullTimeSeconds / traversalSeconds, w);
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

} // namespace CutAccuracy
