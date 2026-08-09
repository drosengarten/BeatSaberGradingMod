#pragma once

#include "CutAccuracy/Scoring.hpp"
#include "scotland2/shared/loader.hpp"

#include <cstddef>
#include <string>

namespace CutAccuracyQuest {

void InitConfig(const modloader::ModInfo& info);
void RegisterSettingsMenu();

CutAccuracy::DifficultyProfile CurrentDifficultyProfile();
CutAccuracy::ScoreWeights CurrentScoreWeights();
const char* CurrentDifficultyName();
bool ShouldShowFlyingScoreText();
std::string FlyingScoreTextForAccuracy(double accuracyPct, std::size_t variantSeed);

} // namespace CutAccuracyQuest
