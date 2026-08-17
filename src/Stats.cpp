#include "CutAccuracy/Stats.hpp"

#include <algorithm>

namespace CutAccuracy {

namespace {

double pct(double earned, double max) {
    return max > 0.0 ? (earned / max) * 100.0 : 0.0;
}

int safeMultiplier(int multiplier) {
    return std::max(0, multiplier);
}

constexpr double defaultFullNoteMax() {
    return 100.0;
}

} // namespace

void SaberStats::reset() {
    notes_ = 0;
    componentSamples_ = 0;
    sums_ = {};
    rawEarned_ = 0.0;
    rawMax_ = 0.0;
    levelEarned_ = 0.0;
    levelMax_ = 0.0;
}

void SaberStats::add(const NoteComponents& c, int actualMultiplier, int maxMultiplier, const ScoreWeights& w) {
    addWeighted(c, defaultFullNoteMax(), actualMultiplier, maxMultiplier, w);
}

void SaberStats::addWeighted(const NoteComponents& c, double objectMaxScore, int actualMultiplier, int maxMultiplier, const ScoreWeights&) {
    if (objectMaxScore <= 0.0) return;

    ++notes_;
    ++componentSamples_;
    sums_.firstMini += c.firstMini;
    sums_.secondMini += c.secondMini;
    sums_.beforeSwing += c.beforeSwing;
    sums_.afterSwing += c.afterSwing;

    const double boundedComponentScore = std::clamp(
        static_cast<double>(roundedClampedToInt(c.total(), 0, 100)), 0.0, defaultFullNoteMax());
    const double objectScore = objectMaxScore * (boundedComponentScore / defaultFullNoteMax());
    const int actual = safeMultiplier(actualMultiplier);
    const int maxPossible = std::max(1, safeMultiplier(maxMultiplier));

    rawEarned_ += objectScore;
    rawMax_ += objectMaxScore;
    levelEarned_ += objectScore * static_cast<double>(actual);
    levelMax_ += objectMaxScore * static_cast<double>(maxPossible);
}

void SaberStats::addFixed(double score, double objectMaxScore, int actualMultiplier, int maxMultiplier) {
    if (objectMaxScore <= 0.0) return;

    ++notes_;
    const double objectScore = std::clamp(score, 0.0, objectMaxScore);
    const int actual = safeMultiplier(actualMultiplier);
    const int maxPossible = std::max(1, safeMultiplier(maxMultiplier));

    rawEarned_ += objectScore;
    rawMax_ += objectMaxScore;
    levelEarned_ += objectScore * static_cast<double>(actual);
    levelMax_ += objectMaxScore * static_cast<double>(maxPossible);
}

void SaberStats::addMiss(int maxMultiplier, const ScoreWeights&) {
    addMissWeighted(defaultFullNoteMax(), maxMultiplier);
}

void SaberStats::addMissWeighted(double objectMaxScore, int maxMultiplier) {
    if (objectMaxScore <= 0.0) return;

    ++notes_;
    const int maxPossible = std::max(1, safeMultiplier(maxMultiplier));
    rawMax_ += objectMaxScore;
    levelMax_ += objectMaxScore * static_cast<double>(maxPossible);
}

SaberAverages SaberStats::averages(const ScoreWeights& w) const {
    if (notes_ == 0) return {};
    const double componentN = static_cast<double>(componentSamples_);
    const double raw = pct(rawEarned_, rawMax_);
    const double level = pct(levelEarned_, levelMax_);
    return {
        notes_,
        componentSamples_,
        raw,
        raw,
        level,
        componentSamples_ == 0 ? 0.0 : (sums_.firstMini / (componentN * w.miniNoteMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.secondMini / (componentN * w.miniNoteMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.beforeSwing / (componentN * w.beforeSwingMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.afterSwing / (componentN * w.afterSwingMax)) * 100.0
    };
}

SessionAverages SessionStats::averages(const ScoreWeights& w) const {
    const auto l = left.averages(w);
    const auto r = right.averages(w);
    const std::size_t totalNotes = l.notes + r.notes;

    const double raw = pct(rawEarned(), rawMax());
    const double level = pct(levelEarned(), levelMax());

    return {totalNotes, raw, raw, level, l, r};
}

} // namespace CutAccuracy
