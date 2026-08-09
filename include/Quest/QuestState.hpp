#pragma once

#include "CutAccuracy/Scoring.hpp"
#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Stats.hpp"
#include "CutAccuracy/Traversal.hpp"

#include <optional>
#include <unordered_map>

namespace GlobalNamespace {
class NoteData;
class SaberMovementData;
class SaberSwingRatingCounter;
}

namespace CutAccuracyQuest {

struct DepthSplitMiniRatios {
    double negativeDepth{0.0};
    double positiveDepth{0.0};
};

struct PendingCut {
    CutAccuracy::SaberSide side{CutAccuracy::SaberSide::Left};
    CutAccuracy::CutDirection cutDirection{CutAccuracy::CutDirection::None};
    DepthSplitMiniRatios firstMiniRatios{};
    DepthSplitMiniRatios secondMiniRatios{};
    CutAccuracy::OrientedBox frozenBox{};
    double cutClockTime{0.0};
};

extern CutAccuracy::SessionStats sessionStats;
extern std::size_t traversalMissingCount;
extern std::size_t dotNotesScored;
extern std::size_t chainLinksScored;
extern std::size_t objectsIgnored;
extern std::size_t unknownScoringTypesIgnored;
extern std::size_t builtinScoreOverridesApplied;
extern int lastBuiltinScoreOverride;
extern int lastBuiltinMaxScoreObserved;
extern std::unordered_map<GlobalNamespace::NoteData*, PendingCut> pendingCuts;
extern std::unordered_map<GlobalNamespace::SaberMovementData*, double> preSwingDegrees;
extern std::unordered_map<GlobalNamespace::SaberSwingRatingCounter*, double> postSwingDegrees;

void ResetSession();

} // namespace CutAccuracyQuest
