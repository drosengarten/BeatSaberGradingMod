#pragma once

#include "CutAccuracy/Scoring.hpp"
#include <cstddef>

namespace CutAccuracy {

enum class SaberSide { Left, Right };

struct SaberAverages {
    std::size_t notes{0};
    std::size_t componentSamples{0};
    double accuracyPct{0.0};
    double rawAccuracyPct{0.0};
    double levelAccuracyPct{0.0};
    double firstMiniPct{0.0};
    double secondMiniPct{0.0};
    double beforeSwingPct{0.0};
    double afterSwingPct{0.0};
};

struct SessionAverages {
    std::size_t notes{0};
    double accuracyPct{0.0};
    double rawAccuracyPct{0.0};
    double levelAccuracyPct{0.0};
    SaberAverages left{};
    SaberAverages right{};
};

class SaberStats {
public:
    void reset();
    void add(const NoteComponents& c, int actualMultiplier = 1, int maxMultiplier = 1, const ScoreWeights& w = {});
    void addWeighted(const NoteComponents& c, double objectMaxScore, int actualMultiplier = 1, int maxMultiplier = 1, const ScoreWeights& w = {});
    void addFixed(double score, double objectMaxScore, int actualMultiplier = 1, int maxMultiplier = 1);
    void addMiss(int maxMultiplier = 1, const ScoreWeights& w = {});
    void addMissWeighted(double objectMaxScore, int maxMultiplier = 1);
    SaberAverages averages(const ScoreWeights& w = {}) const;

    std::size_t notes() const { return notes_; }
    double rawEarned() const { return rawEarned_; }
    double rawMax() const { return rawMax_; }
    double levelEarned() const { return levelEarned_; }
    double levelMax() const { return levelMax_; }

private:
    std::size_t notes_{0};
    std::size_t componentSamples_{0};
    NoteComponents sums_{};
    double rawEarned_{0.0};
    double rawMax_{0.0};
    double levelEarned_{0.0};
    double levelMax_{0.0};
};

struct SessionStats {
    SaberStats left;
    SaberStats right;

    void reset() { left.reset(); right.reset(); }
    SaberStats& forSide(SaberSide s) { return s == SaberSide::Left ? left : right; }
    const SaberStats& forSide(SaberSide s) const { return s == SaberSide::Left ? left : right; }

    double rawEarned() const { return left.rawEarned() + right.rawEarned(); }
    double rawMax() const { return left.rawMax() + right.rawMax(); }
    double levelEarned() const { return left.levelEarned() + right.levelEarned(); }
    double levelMax() const { return left.levelMax() + right.levelMax(); }

    SessionAverages averages(const ScoreWeights& w = {}) const;
};

} // namespace CutAccuracy
