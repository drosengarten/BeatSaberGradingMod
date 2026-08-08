#pragma once

#include "CutAccuracy/Scoring.hpp"

namespace GlobalNamespace {
class CutScoreBuffer;
class FlyingScoreEffect;
class IReadonlyCutScoreBuffer;
}

namespace CutAccuracyQuest {

void RegisterFlyingScore(
    GlobalNamespace::IReadonlyCutScoreBuffer* buffer,
    GlobalNamespace::FlyingScoreEffect* effect);

void PresentCustomFlyingScore(
    GlobalNamespace::CutScoreBuffer* buffer,
    const CutAccuracy::NoteComponents& components);
void PresentFixedFlyingScore(
    GlobalNamespace::CutScoreBuffer* buffer,
    double score,
    double maxScore);

void ReapplyCustomFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer* buffer);
void ReapplyCustomFlyingScore(GlobalNamespace::FlyingScoreEffect* effect);
void ClearFlyingScores();

} // namespace CutAccuracyQuest
