#pragma once

#include "CutAccuracy/Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace CutAccuracy {

enum class ScoringMode : std::uint8_t { Off = 0, Simple = 1, Advanced = 2 };

inline constexpr bool customScoringActiveForMode(ScoringMode mode) {
    return mode == ScoringMode::Simple || mode == ScoringMode::Advanced;
}

inline constexpr bool scoreSubmissionAllowedForMode(ScoringMode mode) {
    return !customScoringActiveForMode(mode);
}

enum class AccuracyMethod : std::uint8_t { BeatSaber = 0, Precise = 1 };
enum class ProfileAccuracyMethod : std::uint8_t { BeatSaber = 0, Precise = 1, Blended = 2 };

enum class FailureMode : std::uint8_t { Zero = 0, FixedPoints = 1, PercentOfMax = 2 };
enum class ComponentId : std::uint8_t { Precise = 0, Center = 1, Before = 2, After = 3, Flat = 4 };

enum class ProfileKind : std::uint8_t {
    DirectionalNote = 0,
    DotNote,
    ArcHead,
    ArcTail,
    ChainHead,
    ChainLink,
    ArcHeadArcTail,
    ChainHeadArcTail,
    ChainHeadArcHead,
    ChainHeadArcHeadArcTail,
    ChainLinkArcHead,
    Count,
    Excluded = 255
};

constexpr std::size_t kProfileCount = static_cast<std::size_t>(ProfileKind::Count);

inline constexpr std::size_t profileIndex(ProfileKind kind) {
    return static_cast<std::size_t>(kind);
}

inline constexpr std::string_view profileKindName(ProfileKind kind) {
    switch (kind) {
        case ProfileKind::DirectionalNote: return "Directional Note";
        case ProfileKind::DotNote: return "Dot Note";
        case ProfileKind::ArcHead: return "Arc Head";
        case ProfileKind::ArcTail: return "Arc Tail";
        case ProfileKind::ChainHead: return "Chain Head";
        case ProfileKind::ChainLink: return "Chain Link";
        case ProfileKind::ArcHeadArcTail: return "Arc Head + Arc Tail";
        case ProfileKind::ChainHeadArcTail: return "Chain Head + Arc Tail";
        case ProfileKind::ChainHeadArcHead: return "Chain Head + Arc Head";
        case ProfileKind::ChainHeadArcHeadArcTail: return "Chain Head + Arc Head + Arc Tail";
        case ProfileKind::ChainLinkArcHead: return "Chain Link + Arc Head";
        default: return "Excluded";
    }
}

inline constexpr ProfileKind profileKindForScoringType(int scoringType, bool dotNote = false) {
    switch (scoringType) {
        case 1: return dotNote ? ProfileKind::DotNote : ProfileKind::DirectionalNote;
        case 2: return ProfileKind::ArcHead;
        case 3: return ProfileKind::ArcTail;
        case 4: return ProfileKind::ChainHead;
        case 5: return ProfileKind::ChainLink;
        case 6: return ProfileKind::ArcHeadArcTail;
        case 7: return ProfileKind::ChainHeadArcTail;
        case 8: return ProfileKind::ChainLinkArcHead;
        case 9: return ProfileKind::ChainHeadArcHead;
        case 10: return ProfileKind::ChainHeadArcHeadArcTail;
        default: return ProfileKind::Excluded;
    }
}

struct WeightedComponent {
    bool enabled{false};
    int weightPct{0};
};

struct PreciseComponent : WeightedComponent {
    // Four mini-notes are always measured. This setting changes only the split
    // between the pair in the note-direction half (Upper) and the opposite pair (Lower).
    int upperPairWeightPct{50};
};

struct SwingComponent : WeightedComponent {
    double fullCreditAngleDeg{60.0};
};

struct FailureRule {
    FailureMode mode{FailureMode::Zero};
    double value{0.0};
};

struct ScoringProfile {
    ProfileAccuracyMethod accuracyMethod{ProfileAccuracyMethod::Precise};
    int preciseMixPct{50};
    double maxScore{100.0};
    // Flat is a normal scoring weight. On a valid cut its quality is always 1.0.
    // A 20% Flat weight therefore contributes 20% of maxScore.
    WeightedComponent flat{false, 0};
    PreciseComponent precise{true, 60, 50};
    WeightedComponent center{false, 0};
    SwingComponent before{true, 20, 60.0};
    SwingComponent after{true, 20, 60.0};
    FailureRule badCut{};
    FailureRule miss{};
};

struct SimpleConfig {
    AccuracyMethod accuracyMethod{AccuracyMethod::Precise};
    double fullNoteMax{100.0};
    double chainLinkMax{20.0};
    int accuracyWeightPct{60};
    int beforeWeightPct{20};
    int afterWeightPct{20};
    double beforeFullCreditAngleDeg{60.0};
    double afterFullCreditAngleDeg{60.0};
    int preciseUpperPairWeightPct{50};
};

struct AdvancedConfig {
    std::array<ScoringProfile, kProfileCount> profiles{};

    ScoringProfile& profile(ProfileKind kind) { return profiles.at(profileIndex(kind)); }
    const ScoringProfile& profile(ProfileKind kind) const { return profiles.at(profileIndex(kind)); }
};

inline int clampPercent(int value) { return std::clamp(value, 0, 100); }
inline double clampPercentDouble(double value) { return std::isfinite(value) ? std::clamp(value, 0.0, 100.0) : 0.0; }
inline double clampQuality(double value) { return std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0; }
inline double clampMaxScore(double value) { return std::isfinite(value) ? std::clamp(value, 0.0, 1000.0) : 0.0; }

inline int roundedNonNegativeToInt(double value) {
    if (!std::isfinite(value) || value <= 0.0) return 0;
    const auto rounded = static_cast<long long>(std::llround(value));
    if (rounded <= 0) return 0;
    if (rounded > static_cast<long long>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(rounded);
}

inline int roundedClampedToInt(double value, int minValue, int maxValue) {
    return std::clamp(roundedNonNegativeToInt(value), minValue, maxValue);
}

inline int customInternalMaxScore(double customLevelMax) {
    return roundedNonNegativeToInt(customLevelMax);
}

inline int customInternalScoreFromCustomLevel(double customLevelEarned, double customLevelMax) {
    const int customMax = customInternalMaxScore(customLevelMax);
    if (customMax <= 0) return 0;
    return std::clamp(roundedNonNegativeToInt(customLevelEarned), 0, customMax);
}

inline double smallerRatio(const MiniNoteVolumes& v) {
    if (v.total <= 1e-12) return 0.0;
    return std::clamp(std::min(v.positiveSide, v.negativeSide) / v.total, 0.0, 0.5);
}

inline double miniQualityFromSmallerRatio(double ratio) {
    return clampQuality(2.0 * std::clamp(ratio, 0.0, 0.5));
}

struct FourMiniNoteQuality {
    double upperNegativeDepth{0.0};
    double upperPositiveDepth{0.0};
    double lowerNegativeDepth{0.0};
    double lowerPositiveDepth{0.0};

    double upperPairQuality() const {
        return 0.5 * (clampQuality(upperNegativeDepth) + clampQuality(upperPositiveDepth));
    }
    double lowerPairQuality() const {
        return 0.5 * (clampQuality(lowerNegativeDepth) + clampQuality(lowerPositiveDepth));
    }
    double preciseQuality(int upperPairWeightPct) const {
        const double upperWeight = static_cast<double>(clampPercent(upperPairWeightPct)) / 100.0;
        return clampQuality(upperPairQuality() * upperWeight + lowerPairQuality() * (1.0 - upperWeight));
    }
};

inline FourMiniNoteQuality fourMiniQualityFromVolumes(
    const DepthSplitMiniNoteVolumes& upper,
    const DepthSplitMiniNoteVolumes& lower) {
    return {
        miniQualityFromSmallerRatio(smallerRatio(upper.negativeDepth)),
        miniQualityFromSmallerRatio(smallerRatio(upper.positiveDepth)),
        miniQualityFromSmallerRatio(smallerRatio(lower.negativeDepth)),
        miniQualityFromSmallerRatio(smallerRatio(lower.positiveDepth))
    };
}

inline double swingQuality(double degrees, double fullCreditAngleDeg) {
    if (!std::isfinite(degrees) || !std::isfinite(fullCreditAngleDeg) || fullCreditAngleDeg <= 1e-9) return 0.0;
    return clampQuality(degrees / fullCreditAngleDeg);
}

inline double failureScore(const FailureRule& rule, double maxScore) {
    maxScore = clampMaxScore(maxScore);
    switch (rule.mode) {
        case FailureMode::FixedPoints:
            return std::isfinite(rule.value) ? std::clamp(rule.value, 0.0, maxScore) : 0.0;
        case FailureMode::PercentOfMax:
            return maxScore * static_cast<double>(std::isfinite(rule.value)
                ? clampPercent(static_cast<int>(std::lround(rule.value)))
                : 0) / 100.0;
        case FailureMode::Zero:
        default:
            return 0.0;
    }
}

inline void normalizeFailureRule(FailureRule& rule, double maxScore) {
    maxScore = clampMaxScore(maxScore);
    switch (rule.mode) {
        case FailureMode::FixedPoints:
            rule.value = std::isfinite(rule.value) ? std::clamp(rule.value, 0.0, maxScore) : 0.0;
            break;
        case FailureMode::PercentOfMax:
            rule.value = std::isfinite(rule.value)
                ? static_cast<double>(clampPercent(static_cast<int>(std::lround(rule.value))))
                : 0.0;
            break;
        case FailureMode::Zero:
        default:
            rule.mode = FailureMode::Zero;
            rule.value = 0.0;
            break;
    }
}

inline WeightedComponent* component(ScoringProfile& profile, ComponentId id) {
    switch (id) {
        case ComponentId::Precise: return &profile.precise;
        case ComponentId::Center: return &profile.center;
        case ComponentId::Before: return &profile.before;
        case ComponentId::After: return &profile.after;
        case ComponentId::Flat: return &profile.flat;
        default: return nullptr;
    }
}

inline const WeightedComponent* component(const ScoringProfile& profile, ComponentId id) {
    return component(const_cast<ScoringProfile&>(profile), id);
}

inline std::array<ComponentId, 5> allComponents() {
    // Flat participates in exactly the same 100% budget as every measured component.
    return {ComponentId::Flat, ComponentId::Precise, ComponentId::Center, ComponentId::Before, ComponentId::After};
}

inline bool hasMeasuredComponents(const ScoringProfile& profile) {
    return (profile.precise.enabled && profile.precise.weightPct > 0) ||
           (profile.center.enabled && profile.center.weightPct > 0) ||
           (profile.before.enabled && profile.before.weightPct > 0) ||
           (profile.after.enabled && profile.after.weightPct > 0);
}

inline bool isChainLinkProfile(ProfileKind kind) {
    return kind == ProfileKind::ChainLink || kind == ProfileKind::ChainLinkArcHead;
}

inline bool usesNativeFixedCarrier(ProfileKind kind, const ScoringProfile& profile) {
    return isChainLinkProfile(kind) && !hasMeasuredComponents(profile);
}

inline int enabledWeightSum(const ScoringProfile& profile) {
    int sum = 0;
    for (auto id : allComponents()) {
        const auto* c = component(profile, id);
        if (c && c->enabled) sum += clampPercent(c->weightPct);
    }
    return sum;
}

inline int componentWeightMaxPct(const ScoringProfile& profile, ComponentId edited) {
    int otherSum = 0;
    for (auto id : allComponents()) {
        if (id == edited) continue;
        const auto* c = component(profile, id);
        if (c && c->enabled) otherSum += clampPercent(c->weightPct);
    }
    return std::clamp(100 - otherSum, 0, 100);
}

// Canonicalize loaded/legacy profiles to whole percentages. Disabled components are
// always zero. Enabled components are rounded to the nearest 1% and then capped in
// a stable component order so the total can never exceed 100%. No proportional
// redistribution is performed.
inline void normalizeEnabledWeights(ScoringProfile& profile) {
    int remaining = 100;
    for (auto id : allComponents()) {
        auto* c = component(profile, id);
        if (!c) continue;
        // In the UI, a zero weight is the canonical OFF state. This avoids a
        // second enable/disable control for every component and keeps profiles
        // visually and semantically consistent.
        if (!c->enabled || c->weightPct <= 0) {
            c->enabled = false;
            c->weightPct = 0;
            continue;
        }
        c->weightPct = std::min(clampPercent(c->weightPct), remaining);
        c->enabled = c->weightPct > 0;
        remaining -= c->weightPct;
    }
}

inline void setComponentWeight(ScoringProfile& profile, ComponentId edited, int requestedPct) {
    auto* target = component(profile, edited);
    if (!target) return;
    const int maxSupported = componentWeightMaxPct(profile, edited);
    target->weightPct = std::clamp(requestedPct, 0, maxSupported);
    target->enabled = target->weightPct > 0;
}

inline int accuracyWeight(const ScoringProfile& p) {
    return (p.center.enabled ? clampPercent(p.center.weightPct) : 0)
         + (p.precise.enabled ? clampPercent(p.precise.weightPct) : 0);
}
inline int accuracyWeightMax(const ScoringProfile& p) {
    return std::max(0,100 - (p.flat.enabled ? clampPercent(p.flat.weightPct) : 0)
        - (p.before.enabled ? clampPercent(p.before.weightPct) : 0)
        - (p.after.enabled ? clampPercent(p.after.weightPct) : 0));
}
inline void setAccuracyWeight(ScoringProfile& p, int requested) {
    const int total=std::clamp(requested,0,accuracyWeightMax(p));
    const int precise=p.accuracyMethod==ProfileAccuracyMethod::Precise ? total
        : p.accuracyMethod==ProfileAccuracyMethod::BeatSaber ? 0
        : static_cast<int>(std::lround(total*clampPercent(p.preciseMixPct)/100.0));
    p.precise.weightPct=precise; p.precise.enabled=precise>0;
    p.center.weightPct=total-precise; p.center.enabled=p.center.weightPct>0;
}
inline void setAccuracyMethod(ScoringProfile& p, ProfileAccuracyMethod method) {
    const int total=accuracyWeight(p);
    p.accuracyMethod=method;
    setAccuracyWeight(p,total);
}

inline void setComponentEnabled(ScoringProfile& profile, ComponentId id, bool enabled, int preferredWeightPct = 20) {
    auto* c = component(profile, id);
    if (!c) return;
    if (!enabled) {
        c->enabled = false;
        c->weightPct = 0;
        return;
    }
    if (c->enabled) return;
    c->enabled = true;
    c->weightPct = 0;
    setComponentWeight(profile, id, preferredWeightPct);
}

inline int simpleWeightMaxPct(const SimpleConfig& cfg, ComponentId edited) {
    int otherSum = 0;
    if (edited != ComponentId::Precise && edited != ComponentId::Center) otherSum += clampPercent(cfg.accuracyWeightPct);
    if (edited != ComponentId::Before) otherSum += clampPercent(cfg.beforeWeightPct);
    if (edited != ComponentId::After) otherSum += clampPercent(cfg.afterWeightPct);
    return std::clamp(100 - otherSum, 0, 100);
}

inline void setSimpleWeight(SimpleConfig& cfg, ComponentId edited, int requestedPct) {
    int* target = nullptr;
    if (edited == ComponentId::Precise || edited == ComponentId::Center) target = &cfg.accuracyWeightPct;
    else if (edited == ComponentId::Before) target = &cfg.beforeWeightPct;
    else target = &cfg.afterWeightPct;
    *target = std::clamp(requestedPct, 0, simpleWeightMaxPct(cfg, edited));
}

inline ScoringProfile profileFromSimple(const SimpleConfig& cfg, ProfileKind kind) {
    ScoringProfile out{};
    out.accuracyMethod=cfg.accuracyMethod==AccuracyMethod::BeatSaber ? ProfileAccuracyMethod::BeatSaber : ProfileAccuracyMethod::Precise;
    if (kind == ProfileKind::ChainLink || kind == ProfileKind::ChainLinkArcHead) {
        out.maxScore = clampMaxScore(cfg.chainLinkMax);
        out.flat = {true, 100};
        out.precise = {false, 0, clampPercent(cfg.preciseUpperPairWeightPct)};
        out.center = {false, 0};
        out.before = {false, 0, cfg.beforeFullCreditAngleDeg};
        out.after = {false, 0, cfg.afterFullCreditAngleDeg};
        return out;
    }

    out.maxScore = clampMaxScore(cfg.fullNoteMax);
    out.flat = {false, 0};
    out.precise = {
        cfg.accuracyMethod == AccuracyMethod::Precise,
        cfg.accuracyMethod == AccuracyMethod::Precise ? clampPercent(cfg.accuracyWeightPct) : 0,
        clampPercent(cfg.preciseUpperPairWeightPct)
    };
    out.center = {
        cfg.accuracyMethod == AccuracyMethod::BeatSaber,
        cfg.accuracyMethod == AccuracyMethod::BeatSaber ? clampPercent(cfg.accuracyWeightPct) : 0
    };
    out.before = {true, clampPercent(cfg.beforeWeightPct), std::clamp(cfg.beforeFullCreditAngleDeg, 1.0, 180.0)};
    out.after = {true, clampPercent(cfg.afterWeightPct), std::clamp(cfg.afterFullCreditAngleDeg, 1.0, 180.0)};
    normalizeEnabledWeights(out);
    return out;
}

inline AdvancedConfig advancedFromSimple(const SimpleConfig& cfg) {
    AdvancedConfig out{};
    for (std::size_t i = 0; i < kProfileCount; ++i) {
        out.profiles[i] = profileFromSimple(cfg, static_cast<ProfileKind>(i));
    }
    return out;
}

inline ScoringProfile vanillaLikeProfile(
    int maxScore,
    int flatWeightPct,
    int centerWeightPct,
    int beforeWeightPct,
    int afterWeightPct) {
    ScoringProfile out{};
    out.accuracyMethod=ProfileAccuracyMethod::BeatSaber;
    out.maxScore = clampMaxScore(maxScore);
    out.flat = {flatWeightPct > 0, clampPercent(flatWeightPct)};
    out.precise = {false, 0, 50};
    out.center = {centerWeightPct > 0, clampPercent(centerWeightPct)};
    out.before = {beforeWeightPct > 0, clampPercent(beforeWeightPct), 100.0};
    out.after = {afterWeightPct > 0, clampPercent(afterWeightPct), 60.0};
    out.badCut = {FailureMode::Zero, 0.0};
    out.miss = {FailureMode::Zero, 0.0};
    normalizeEnabledWeights(out);
    return out;
}

inline AdvancedConfig defaultAdvancedConfig() {
    AdvancedConfig out{};
    // Beat Saber-like defaults expressed as one whole-percent budget. Flat is
    // simply the guaranteed share of maxScore for a valid cut. The native point
    // ratios are rounded to the nearest 1%; each default profile totals 100%.
    out.profile(ProfileKind::DirectionalNote) = vanillaLikeProfile(115, 0, 13, 61, 26);
    out.profile(ProfileKind::DotNote) = vanillaLikeProfile(115, 0, 13, 61, 26);
    out.profile(ProfileKind::ArcHead) = vanillaLikeProfile(115, 26, 13, 61, 0);
    out.profile(ProfileKind::ArcTail) = vanillaLikeProfile(115, 61, 13, 0, 26);
    out.profile(ProfileKind::ChainHead) = vanillaLikeProfile(85, 0, 18, 82, 0);
    out.profile(ProfileKind::ChainLink) = vanillaLikeProfile(20, 100, 0, 0, 0);
    out.profile(ProfileKind::ArcHeadArcTail) = vanillaLikeProfile(115, 87, 13, 0, 0);
    out.profile(ProfileKind::ChainHeadArcTail) = vanillaLikeProfile(115, 87, 13, 0, 0);
    out.profile(ProfileKind::ChainHeadArcHead) = vanillaLikeProfile(115, 26, 13, 61, 0);
    out.profile(ProfileKind::ChainHeadArcHeadArcTail) = vanillaLikeProfile(115, 87, 13, 0, 0);
    out.profile(ProfileKind::ChainLinkArcHead) = vanillaLikeProfile(20, 100, 0, 0, 0);
    return out;
}

struct ScoringInput {
    FourMiniNoteQuality mini{};
    double centerQuality{0.0};
    double beforeSwingDeg{0.0};
    double afterSwingDeg{0.0};
    bool preciseAvailable{false};
};

struct ScoredNote {
    double accuracyQuality{0.0};
    bool accuracyEnabled{false};
    double maxScore{0.0};
    double flatPoints{0.0};
    double totalScore{0.0};

    double preciseQuality{0.0};
    double centerQuality{0.0};
    double beforeQuality{0.0};
    double afterQuality{0.0};
    double upperPairQuality{0.0};
    double lowerPairQuality{0.0};

    double precisePoints{0.0};
    double centerPoints{0.0};
    double beforePoints{0.0};
    double afterPoints{0.0};

    bool preciseEnabled{false};
    bool centerEnabled{false};
    bool beforeEnabled{false};
    bool afterEnabled{false};

    double normalizedScore() const {
        return maxScore > 1e-12 ? clampQuality(totalScore / maxScore) : 0.0;
    }
};

inline ScoredNote scoreValidCut(const ScoringInput& input, ScoringProfile profile) {
    profile.maxScore = clampMaxScore(profile.maxScore);
    profile.precise.upperPairWeightPct = clampPercent(profile.precise.upperPairWeightPct);
    profile.before.fullCreditAngleDeg = std::clamp(profile.before.fullCreditAngleDeg, 1.0, 180.0);
    profile.after.fullCreditAngleDeg = std::clamp(profile.after.fullCreditAngleDeg, 1.0, 180.0);
    normalizeEnabledWeights(profile);

    ScoredNote out{};
    out.maxScore = profile.maxScore;
    out.flatPoints = profile.flat.enabled
        ? profile.maxScore * static_cast<double>(clampPercent(profile.flat.weightPct)) / 100.0
        : 0.0;
    out.preciseEnabled = profile.precise.enabled;
    out.centerEnabled = profile.center.enabled;
    out.beforeEnabled = profile.before.enabled;
    out.afterEnabled = profile.after.enabled;

    out.upperPairQuality = input.preciseAvailable ? input.mini.upperPairQuality() : 0.0;
    out.lowerPairQuality = input.preciseAvailable ? input.mini.lowerPairQuality() : 0.0;
    out.preciseQuality = input.preciseAvailable
        ? input.mini.preciseQuality(profile.precise.upperPairWeightPct)
        : 0.0;
    out.centerQuality = clampQuality(input.centerQuality);
    const int accuracyShare=accuracyWeight(profile);
    out.accuracyEnabled=accuracyShare>0;
    out.accuracyQuality=accuracyShare>0
        ? (out.preciseQuality*profile.precise.weightPct + out.centerQuality*profile.center.weightPct)/accuracyShare : 0.0;
    out.beforeQuality = swingQuality(input.beforeSwingDeg, profile.before.fullCreditAngleDeg);
    out.afterQuality = swingQuality(input.afterSwingDeg, profile.after.fullCreditAngleDeg);

    auto contribution = [&](const WeightedComponent& c, double quality) {
        if (!c.enabled || out.maxScore <= 0.0) return 0.0;
        return out.maxScore * static_cast<double>(clampPercent(c.weightPct)) / 100.0 * clampQuality(quality);
    };

    out.precisePoints = contribution(profile.precise, out.preciseQuality);
    out.centerPoints = contribution(profile.center, out.centerQuality);
    out.beforePoints = contribution(profile.before, out.beforeQuality);
    out.afterPoints = contribution(profile.after, out.afterQuality);
    out.totalScore = std::clamp(
        out.flatPoints + out.precisePoints + out.centerPoints + out.beforePoints + out.afterPoints,
        0.0,
        out.maxScore);
    return out;
}

struct BeatSaberCutScoreParts {
    int centerDistance{0};
    int before{0};
    int after{0};
    int fixed{0};
};

struct BeatSaberCarrierDefinition {
    int centerDistanceMax{0};
    int beforeMax{0};
    int afterMax{0};
    int fixed{0};
    int maxScore() const { return centerDistanceMax + beforeMax + afterMax + fixed; }
};

inline BeatSaberCarrierDefinition beatSaberMeasurementCarrier(double customMaxScore) {
    const int total = roundedClampedToInt(customMaxScore, 0, 1000);
    if (total <= 0) return {};
    const int center = std::min(15, total);
    const int remaining = total - center;
    const int before = static_cast<int>(std::lround(static_cast<double>(remaining) * 70.0 / 100.0));
    const int after = remaining - before;
    return {center, before, after, 0};
}

inline BeatSaberCarrierDefinition beatSaberFixedCarrier(double customMaxScore) {
    return {0, 0, 0, roundedClampedToInt(customMaxScore, 0, 1000)};
}

inline BeatSaberCutScoreParts beatSaberCutScoreParts(const ScoredNote& scored, BeatSaberCarrierDefinition carrier) {
    // Encode the custom percentage into the same maximum that Beat Saber sees
    // for this object. That keeps native level accuracy displays aligned with
    // the custom score/max fields written into ScoreController.
    const int carrierMax = std::max(0, carrier.maxScore() - carrier.fixed);
    const int total = roundedClampedToInt(scored.normalizedScore() * static_cast<double>(carrierMax), 0, carrierMax);
    const int before = std::min(total, std::max(0, carrier.beforeMax));
    const int after = std::min(total - before, std::max(0, carrier.afterMax));
    const int center = std::min(total - before - after, std::max(0, carrier.centerDistanceMax));
    return {center, before, after, 0};
}

inline BeatSaberCutScoreParts beatSaberCutScoreParts(const ScoredNote& scored) {
    return beatSaberCutScoreParts(scored, {0, 70, 30, 0});
}

} // namespace CutAccuracy
