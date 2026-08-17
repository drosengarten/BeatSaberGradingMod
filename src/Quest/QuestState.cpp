#include "Quest/QuestState.hpp"

namespace CutAccuracyQuest {

CutAccuracy::SessionStats sessionStats{};
std::size_t dotNotesScored{0};
std::size_t chainLinksScored{0};
std::size_t objectsIgnored{0};
std::size_t unknownScoringTypesIgnored{0};
std::size_t builtinScoreOverridesApplied{0};
int lastBuiltinScoreOverride{0};
int lastBuiltinMaxScoreObserved{0};
std::unordered_map<GlobalNamespace::NoteData*, PendingCut> pendingCuts{};
std::unordered_map<GlobalNamespace::SaberMovementData*, double> preSwingDegrees{};
std::unordered_map<GlobalNamespace::SaberSwingRatingCounter*, double> postSwingDegrees{};

void ResetSession() {
    sessionStats.reset();
    dotNotesScored = 0;
    chainLinksScored = 0;
    objectsIgnored = 0;
    unknownScoringTypesIgnored = 0;
    builtinScoreOverridesApplied = 0;
    lastBuiltinScoreOverride = 0;
    lastBuiltinMaxScoreObserved = 0;
    pendingCuts.clear();
    preSwingDegrees.clear();
    postSwingDegrees.clear();
}

} // namespace CutAccuracyQuest
