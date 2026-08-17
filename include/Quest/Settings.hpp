#pragma once

#include "CutAccuracy/Scoring.hpp"
#include "scotland2/shared/loader.hpp"

#include <string>

namespace CutAccuracyQuest {

void InitConfig(const modloader::ModInfo& info);
void RegisterSettingsMenu();

CutAccuracy::ScoreWeights CurrentScoreWeights();
int CurrentAccuracyWeightPercent();
bool ShouldShowFlyingScoreText();
std::string FlyingScoreTextForAccuracy(double accuracyPct);

} // namespace CutAccuracyQuest
