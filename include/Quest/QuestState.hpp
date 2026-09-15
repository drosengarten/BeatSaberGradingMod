#pragma once
#include "CutAccuracy/Scoring.hpp"
#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Stats.hpp"
#include <unordered_map>
namespace GlobalNamespace { class NoteData; class SaberMovementData; class SaberSwingRatingCounter; }
namespace CutAccuracyQuest {
struct DepthSplitMiniRatios { double negativeDepth{0},positiveDepth{0}; };
struct PendingCut {
 CutAccuracy::SaberSide side{CutAccuracy::SaberSide::Left};
 CutAccuracy::CutDirection cutDirection{CutAccuracy::CutDirection::None};
 // Upper = positive half of the SIGNED note-direction axis; Lower = opposite half.
 DepthSplitMiniRatios upperRatios{};
 DepthSplitMiniRatios lowerRatios{};
};
extern CutAccuracy::SessionStats sessionStats;
extern std::size_t dotNotesScored,chainLinksScored,objectsIgnored,unknownScoringTypesIgnored,builtinScoreOverridesApplied;
extern int lastBuiltinScoreOverride,lastBuiltinMaxScoreObserved;
extern std::unordered_map<GlobalNamespace::NoteData*,PendingCut> pendingCuts;
extern std::unordered_map<GlobalNamespace::SaberMovementData*,double> preSwingDegrees;
extern std::unordered_map<GlobalNamespace::SaberSwingRatingCounter*,double> postSwingDegrees;
void ResetSession();
}
