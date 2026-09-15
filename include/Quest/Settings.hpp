#pragma once
#include "CutAccuracy/Scoring.hpp"
#include "scotland2/shared/loader.hpp"
#include <string>
namespace CutAccuracyQuest {
void InitConfig(const modloader::ModInfo& info);
void RegisterSettingsMenu();
CutAccuracy::ScoringMode CurrentScoringMode();
CutAccuracy::AccuracyMethod CurrentSimpleAccuracyMethod();
CutAccuracy::SimpleConfig CurrentSimpleConfig();
const CutAccuracy::AdvancedConfig& CurrentAdvancedConfig();
CutAccuracy::ScoringProfile CurrentProfile(CutAccuracy::ProfileKind kind);
bool CustomScoringActive();
bool ShouldShowFlyingScore();
bool ShouldShowFlyingScoreText();
std::string FlyingScoreTextForAccuracy(double accuracyPct);
}
