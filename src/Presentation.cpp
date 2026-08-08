#include "CutAccuracy/Presentation.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace CutAccuracy {
namespace {

void metric(char* out, std::size_t outSize, std::size_t samples, double value) {
    if (samples == 0) std::snprintf(out, outSize, "  -- ");
    else std::snprintf(out, outSize, "<color=%s>%5.1f</color>", accuracyBandHex(value), value);
}

void speedMetric(char* out, std::size_t outSize, const SaberAverages& avg) {
    if (avg.speedSamples == 0) std::snprintf(out, outSize, "  -- ");
    else std::snprintf(out, outSize, "<color=%s>%5.1f</color>", accuracyBandHex(avg.speedPct), avg.speedPct);
}

std::string coloredPct(double value) {
    char number[64];
    std::snprintf(number, sizeof(number), "<color=%s>%6.2f%%</color>", accuracyBandHex(value), value);
    return number;
}

} // namespace

HudPresentation buildHudPresentation(const SessionStats& stats) {
    const auto summary = stats.averages();

    char heading[128];
    if (summary.notes == 0) {
        std::snprintf(heading, sizeof(heading), "LEVEL ACC  --\n RAW ACC   --");
    } else {
        const auto level = coloredPct(summary.levelAccuracyPct);
        const auto raw = coloredPct(summary.rawAccuracyPct);
        std::snprintf(heading, sizeof(heading), "LEVEL ACC %s\n RAW ACC  %s",
                      level.c_str(), raw.c_str());
    }

    char lu[64], ru[64], ll[64], rl[64], lb[64], rb[64], la[64], ra[64], ls[64], rs[64];
    metric(lu, sizeof(lu), summary.left.componentSamples, summary.left.firstMiniPct);
    metric(ru, sizeof(ru), summary.right.componentSamples, summary.right.firstMiniPct);
    metric(ll, sizeof(ll), summary.left.componentSamples, summary.left.secondMiniPct);
    metric(rl, sizeof(rl), summary.right.componentSamples, summary.right.secondMiniPct);
    metric(lb, sizeof(lb), summary.left.componentSamples, summary.left.beforeSwingPct);
    metric(rb, sizeof(rb), summary.right.componentSamples, summary.right.beforeSwingPct);
    metric(la, sizeof(la), summary.left.componentSamples, summary.left.afterSwingPct);
    metric(ra, sizeof(ra), summary.right.componentSamples, summary.right.afterSwingPct);
    speedMetric(ls, sizeof(ls), summary.left);
    speedMetric(rs, sizeof(rs), summary.right);

    char table[512];
    std::snprintf(table, sizeof(table),
        "          L       R\n"
        "Upper   %s   %s\n"
        "Lower   %s   %s\n"
        "Before  %s   %s\n"
        "After   %s   %s\n"
        "Speed   %s   %s",
        lu, ru, ll, rl, lb, rb, la, ra, ls, rs);

    return {summary.levelAccuracyPct, heading, table};
}

std::string formatPerNoteScore(const NoteComponents& components) {
    char score[16];
    std::snprintf(score, sizeof(score), "%.0f", std::clamp(components.total(), 0.0, 100.0));
    return score;
}

std::string formatFixedScore(double score, double maxScore) {
    char out[16];
    const double bounded = maxScore > 0.0 ? std::clamp(score, 0.0, maxScore) : 0.0;
    std::snprintf(out, sizeof(out), "%.0f", bounded);
    return out;
}

int accuracyBand5(double accuracyPct) {
    const double clamped = std::clamp(accuracyPct, 0.0, 100.0);
    return static_cast<int>(clamped / 5.0) * 5;
}

const char* accuracyBandHex(double accuracyPct) {
    static constexpr const char* bands[] = {
        "#ff0038", "#ff102f", "#ff2025", "#ff321b", "#ff4612",
        "#ff5a08", "#ff7000", "#ff8700", "#ff9f00", "#ffb800",
        "#ffd200", "#ffee00", "#e2ff00", "#c4ff00", "#a5ff00",
        "#83ff00", "#5fff00", "#38ff00", "#00ff2f", "#00ff66",
        "#00ffaa"
    };
    return bands[accuracyBand5(accuracyPct) / 5];
}

AccuracyRgb accuracyBandRgb(double accuracyPct) {
    static constexpr AccuracyRgb bands[] = {
        {1.000f, 0.000f, 0.220f}, {1.000f, 0.063f, 0.184f}, {1.000f, 0.125f, 0.145f},
        {1.000f, 0.196f, 0.106f}, {1.000f, 0.275f, 0.071f}, {1.000f, 0.353f, 0.031f},
        {1.000f, 0.439f, 0.000f}, {1.000f, 0.529f, 0.000f}, {1.000f, 0.624f, 0.000f},
        {1.000f, 0.722f, 0.000f}, {1.000f, 0.824f, 0.000f}, {1.000f, 0.933f, 0.000f},
        {0.886f, 1.000f, 0.000f}, {0.769f, 1.000f, 0.000f}, {0.647f, 1.000f, 0.000f},
        {0.514f, 1.000f, 0.000f}, {0.373f, 1.000f, 0.000f}, {0.220f, 1.000f, 0.000f},
        {0.000f, 1.000f, 0.184f}, {0.000f, 1.000f, 0.400f}, {0.000f, 1.000f, 0.667f}
    };
    return bands[accuracyBand5(accuracyPct) / 5];
}

} // namespace CutAccuracy
