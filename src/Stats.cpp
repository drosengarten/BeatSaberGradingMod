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

double defaultFullNoteMax(const ScoreWeights& w) {
    return 2*w.miniNoteMax + w.beforeSwingMax + w.afterSwingMax + w.speedMax;
}

double beatSaberIntegerScore(const NoteComponents& c, const ScoreWeights& w) {
    const auto parts = beatSaberCutScoreParts(c, w);
    return static_cast<double>(parts.centerDistance + parts.before + parts.after + parts.fixed);
}

} // namespace

void SaberStats::reset() {
    notes_ = 0;
    componentSamples_ = 0;
    speedSamples_ = 0;
    sums_ = {};
    rawEarned_ = 0.0;
    rawMax_ = 0.0;
    levelEarned_ = 0.0;
    levelMax_ = 0.0;
}

void SaberStats::add(const NoteComponents& c, int actualMultiplier, int maxMultiplier, const ScoreWeights& w) {
    addWeighted(c, defaultFullNoteMax(w), actualMultiplier, maxMultiplier, w);
}

void SaberStats::addWeighted(const NoteComponents& c, double objectMaxScore, int actualMultiplier, int maxMultiplier, const ScoreWeights& w) {
    if (objectMaxScore <= 0.0) return;

    const double componentMax = defaultFullNoteMax(w);
    if (componentMax <= 0.0) return;

    ++notes_;
    ++componentSamples_;
    sums_.firstMini += c.firstMini;
    sums_.secondMini += c.secondMini;
    sums_.beforeSwing += c.beforeSwing;
    sums_.afterSwing += c.afterSwing;
    if (c.speedObserved) {
        ++speedSamples_;
        sums_.speed += c.speed;
    }

    const double boundedComponentScore = std::clamp(beatSaberIntegerScore(c, w), 0.0, componentMax);
    const double objectScore = objectMaxScore * (boundedComponentScore / componentMax);
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

void SaberStats::addMiss(int maxMultiplier, const ScoreWeights& w) {
    addMissWeighted(defaultFullNoteMax(w), maxMultiplier);
}

void SaberStats::addMissWeighted(double objectMaxScore, int maxMultiplier) {
    if (objectMaxScore <= 0.0) return;

    ++notes_; // zero contribution: misses remain part of raw/level accuracy.
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
        speedSamples_,
        raw,
        raw,
        level,
        componentSamples_ == 0 ? 0.0 : (sums_.firstMini / (componentN * w.miniNoteMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.secondMini / (componentN * w.miniNoteMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.beforeSwing / (componentN * w.beforeSwingMax)) * 100.0,
        componentSamples_ == 0 ? 0.0 : (sums_.afterSwing / (componentN * w.afterSwingMax)) * 100.0,
        speedSamples_ == 0 ? 0.0 : (sums_.speed / (static_cast<double>(speedSamples_) * w.speedMax)) * 100.0
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
