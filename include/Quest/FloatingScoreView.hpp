#pragma once
#include "CutAccuracy/Scoring.hpp"
namespace GlobalNamespace{class CutScoreBuffer;class FlyingScoreEffect;class IReadonlyCutScoreBuffer;}
namespace CutAccuracyQuest{
void RegisterFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer*,GlobalNamespace::FlyingScoreEffect*);
void PresentCustomFlyingScore(GlobalNamespace::CutScoreBuffer*,const CutAccuracy::ScoredNote&);
void PresentFixedFlyingScore(GlobalNamespace::CutScoreBuffer*,double,double);
void ReapplyCustomFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer*);void ReapplyCustomFlyingScore(GlobalNamespace::FlyingScoreEffect*);void ClearFlyingScores();
}
