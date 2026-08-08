#include "main.hpp"
#include "Quest/QuestState.hpp"
#include "Quest/UnityAdapters.hpp"
#include "Quest/HudModel.hpp"
#include "Quest/FloatingScoreView.hpp"

#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Scoring.hpp"
#include "CutAccuracy/Traversal.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/delegate.hpp"

#include "GlobalNamespace/BeatmapObjectManager.hpp"
#include "GlobalNamespace/BladeMovementDataElement.hpp"
#include "GlobalNamespace/GoodCutScoringElement.hpp"
#include "GlobalNamespace/BadCutScoringElement.hpp"
#include "GlobalNamespace/MissScoringElement.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutDirection.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "GlobalNamespace/SaberSwingRatingCounter.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "GlobalNamespace/FlyingScoreEffect.hpp"
#include "GlobalNamespace/IReadonlyCutScoreBuffer.hpp"
#include "GlobalNamespace/ScoringElement.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"

#include "System/Action_1.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Color.hpp"

#include "beatsaber-hook/shared/utils/byref.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace GlobalNamespace;

namespace CutAccuracyQuest {
namespace {

System::Action_1<ScoringElement*>* scoreFinishedDelegate = nullptr;
ScoreController* scoreControllerWithDelegate = nullptr;

UnityEngine::Vector3 Add(UnityEngine::Vector3 lhs, UnityEngine::Vector3 rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

UnityEngine::Vector3 Subtract(UnityEngine::Vector3 lhs, UnityEngine::Vector3 rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

UnityEngine::Vector3 Scale(UnityEngine::Vector3 value, float scalar) {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

void PatchScoreDefinition(ScoreModel_NoteScoreDefinition* def, const CutAccuracy::ScoreObjectRule& rule) {
    if (!def) return;

    if (rule.kind == CutAccuracy::ScoreObjectKind::FullNote) {
        // Rewrite the built-in score definition so ScoreModel's max-score graph
        // uses CutAccuracy's full-note denominator of 100 instead of vanilla
        // 115. The exact component split here is only for Beat Saber's max/score
        // model; the actual earned score is still overwritten from CutAccuracy's
        // own mini-note/swing/speed calculation after each scoring event.
        def->___maxCenterDistanceCutScore = 50;
        def->___minBeforeCutScore = 0;
        def->___maxBeforeCutScore = 20;
        def->___minAfterCutScore = 0;
        def->___maxAfterCutScore = 20;
        def->___fixedCutScore = 10;
        return;
    }

    if (rule.kind == CutAccuracy::ScoreObjectKind::ChainLink) {
        // Chain links intentionally remain Beat Saber-like fixed 20/0 objects.
        def->___maxCenterDistanceCutScore = 0;
        def->___minBeforeCutScore = 0;
        def->___maxBeforeCutScore = 0;
        def->___minAfterCutScore = 0;
        def->___maxAfterCutScore = 0;
        def->___fixedCutScore = 20;
        return;
    }

    // NoScore and other excluded score definitions contribute no denominator.
    def->___maxCenterDistanceCutScore = 0;
    def->___minBeforeCutScore = 0;
    def->___maxBeforeCutScore = 0;
    def->___minAfterCutScore = 0;
    def->___maxAfterCutScore = 0;
    def->___fixedCutScore = 0;
}

CutAccuracy::SaberSide ExpectedSide(NoteData* note) {
    // Beat Saber NoteData uses the two normal note types as A=0 and B=1.
    // This keeps misses and wrong-saber cuts charged to the saber intended by the map.
    return static_cast<int>(note->colorType) == 0
        ? CutAccuracy::SaberSide::Left
        : CutAccuracy::SaberSide::Right;
}

CutAccuracy::CutDirection ToCoreCutDirection(NoteCutDirection direction) {
    switch (static_cast<int>(direction)) {
        case 0: return CutAccuracy::CutDirection::Up;
        case 1: return CutAccuracy::CutDirection::Down;
        case 2: return CutAccuracy::CutDirection::Left;
        case 3: return CutAccuracy::CutDirection::Right;
        case 4: return CutAccuracy::CutDirection::UpLeft;
        case 5: return CutAccuracy::CutDirection::UpRight;
        case 6: return CutAccuracy::CutDirection::DownLeft;
        case 7: return CutAccuracy::CutDirection::DownRight;
        case 8: return CutAccuracy::CutDirection::Any;
        case 9: return CutAccuracy::CutDirection::None;
        default: return CutAccuracy::CutDirection::None;
    }
}

int ActualMultiplier(ScoringElement* element) {
    return element ? std::max(0, element->get_multiplier()) : 0;
}

int MaxMultiplier(ScoringElement* element) {
    return element ? std::max(1, element->get_maxMultiplier()) : 1;
}

CutAccuracy::ScoreObjectRule RuleForNote(NoteData* note) {
    if (!note) return {};

    const int color = static_cast<int>(note->colorType);
    if (color != 0 && color != 1) return {}; // hazards/unhittable/non-colored objects never affect accuracy.

    auto rule = CutAccuracy::scoreObjectRuleForScoringType(static_cast<int>(note->scoringType));
    if (rule.kind == CutAccuracy::ScoreObjectKind::Excluded) return rule;

    const auto direction = ToCoreCutDirection(note->cutDirection);
    if (rule.usesCubeModel && direction == CutAccuracy::CutDirection::None) {
        return {CutAccuracy::ScoreObjectKind::Excluded, 0.0, false, false, "NoCutDirection"};
    }
    return rule;
}

bool IsTrackedScoringObject(NoteData* note) {
    return RuleForNote(note).kind != CutAccuracy::ScoreObjectKind::Excluded;
}

bool UsesCubeModel(NoteData* note) {
    return RuleForNote(note).usesCubeModel;
}

CutAccuracy::Vec3 ResolveSplitAxisLocal(
    NoteData* note,
    UnityEngine::Transform* noteTransform,
    const NoteCutInfo& noteCutInfo
) {
    if (!note || !noteTransform) return {0,0,0};

    const auto direction = ToCoreCutDirection(note->cutDirection);

    // Dot notes: no map-specified cut direction exists. Score them as genuine
    // normal notes by defining the split axis from the player's actual saber
    // travel direction at the cut. Misses still count as zero through CommitMiss.
    if (direction == CutAccuracy::CutDirection::Any) {
        auto local = LocalDirectionToNotePlane(noteTransform, noteCutInfo.saberDir);
        if (CutAccuracy::lengthSq(local) > 1e-8) {
            ++dotNotesScored;
            return local;
        }
        return {0,0,0};
    }

    if (!CutAccuracy::hasDirectionalSplit(direction)) return {0,0,0};

    const auto worldAxis = CutAccuracy::splitAxisForCutDirection(
        direction, static_cast<double>(note->cutDirectionAngleOffset));

    // Beat Saber note transforms may already be arrow-aligned. If local +Y
    // points along the map's cut axis in world space, use local +Y and avoid
    // double-applying the note direction. If not, convert the map direction
    // into the current note local frame. This handles both stock and altered
    // note transform conventions more safely than either assumption alone.
    const auto localUpWorld = LocalNoteUpWorld(noteTransform);
    const double aligned = std::abs(CutAccuracy::dot(localUpWorld, CutAccuracy::normalized(worldAxis)));
    if (aligned > 0.75) return {0, 1, 0};

    auto localAxis = WorldDirectionToLocalNotePlane(noteTransform, worldAxis);
    if (CutAccuracy::lengthSq(localAxis) > 1e-8) return localAxis;
    return {0, 1, 0};
}

std::vector<CutAccuracy::SaberPlaneSample> MovementSamples(SaberMovementData* movement) {
    std::vector<CutAccuracy::SaberPlaneSample> result;
    if (!movement) return result;

    const auto data = movement->_data;
    const int length = data.size();
    const int valid = std::min(movement->_validCount, length);
    if (length <= 0 || valid <= 0) return result;

    int index = movement->_nextAddIndex - valid;
    while (index < 0) index += length;

    result.reserve(valid);
    for (int i = 0; i < valid; ++i) {
        const auto& e = data[index];
        const auto midpoint = Scale(Add(e.topPos, e.bottomPos), 0.5f);
        result.push_back({
            static_cast<double>(e.time),
            ToCore(midpoint),
            ToCore(e.segmentNormal)
        });
        index = (index + 1) % length;
    }
    return result;
}

void SyncBuiltinScoreOverride() {
    if (!scoreControllerWithDelegate) return;

    // Option B true-internal mode: overwrite both Beat Saber's current score and
    // Beat Saber's current max score with CutAccuracy's integer score space. This
    // prevents the 100/115 mismatch and also prevents the score display from being
    // merely a vanilla-denominator projection.
    const int customMax = CutAccuracy::customInternalMaxScore(sessionStats.levelMax());
    if (customMax <= 0) return;

    const int overrideScore = CutAccuracy::customInternalScoreFromCustomLevel(
        sessionStats.levelEarned(), sessionStats.levelMax());

    scoreControllerWithDelegate->_multipliedScore = overrideScore;
    scoreControllerWithDelegate->_immediateMaxPossibleMultipliedScore = customMax;
    lastBuiltinScoreOverride = overrideScore;
    lastBuiltinMaxScoreObserved = customMax;
    ++builtinScoreOverridesApplied;
}

void CommitMiss(NoteData* note, int maxMultiplier = 1) {
    const auto rule = RuleForNote(note);
    if (rule.kind == CutAccuracy::ScoreObjectKind::Excluded) {
        ++objectsIgnored;
        if (rule.name && std::string(rule.name) == "Unknown") ++unknownScoringTypesIgnored;
        pendingCuts.erase(note);
        return;
    }

    sessionStats.forSide(ExpectedSide(note)).addMissWeighted(rule.maxScore, maxMultiplier);
    pendingCuts.erase(note);
    SyncBuiltinScoreOverride();
    MarkHudDirty();
}

void CommitGood(GoodCutScoringElement* good) {
    if (!good || !good->noteData) return;
    auto* note = good->noteData;
    const auto rule = RuleForNote(note);
    if (rule.kind == CutAccuracy::ScoreObjectKind::Excluded) {
        ++objectsIgnored;
        if (rule.name && std::string(rule.name) == "Unknown") ++unknownScoringTypesIgnored;
        pendingCuts.erase(note);
        return;
    }

    auto* buffer = good->_cutScoreBuffer;

    if (rule.kind == CutAccuracy::ScoreObjectKind::ChainLink) {
        sessionStats.forSide(ExpectedSide(note)).addFixed(rule.maxScore, rule.maxScore, ActualMultiplier(good), MaxMultiplier(good));
        SyncBuiltinScoreOverride();
        ++chainLinksScored;
        PresentFixedFlyingScore(buffer, rule.maxScore, rule.maxScore);
        CutAccuracyLogger.info(
            "chain link {:.0f}/{:.0f} | raw {:.2f}% level {:.2f}% | mult {}/{}",
            rule.maxScore, rule.maxScore, sessionStats.averages().rawAccuracyPct,
            sessionStats.averages().levelAccuracyPct, ActualMultiplier(good), MaxMultiplier(good));
        MarkHudDirty();
        return;
    }

    const auto it = pendingCuts.find(note);
    if (it == pendingCuts.end()) {
        CutAccuracyLogger.warn("No pending cut geometry for {}; counting zero", rule.name);
        CommitMiss(note, MaxMultiplier(good));
        return;
    }

    const PendingCut pending = it->second;
    pendingCuts.erase(it);

    auto* counter = buffer ? buffer->_saberSwingRatingCounter : nullptr;
    auto* movement = counter ? (SaberMovementData*)counter->_saberMovementData : nullptr;

    const double beforeDeg = movement && preSwingDegrees.contains(movement)
        ? preSwingDegrees[movement] : 0.0;
    const double afterDeg = counter && postSwingDegrees.contains(counter)
        ? postSwingDegrees[counter] : 0.0;

    std::optional<double> traversal;
    if (movement) {
        const auto samples = MovementSamples(movement);
        if (auto measured = CutAccuracy::traversalTimeSeconds(
                samples, pending.frozenBox, pending.cutClockTime)) {
            traversal = *measured;
        }
    }

    const CutAccuracy::RawMeasurements raw{
        pending.firstMiniRatio,
        pending.secondMiniRatio,
        beforeDeg,
        afterDeg,
        traversal.value_or(0.0)
    };
    auto components = traversal ? CutAccuracy::score(raw) : CutAccuracy::scoreWithUnknownSpeed(raw);
    if (!traversal) ++traversalMissingCount;

    sessionStats.forSide(pending.side).addWeighted(components, rule.maxScore, ActualMultiplier(good), MaxMultiplier(good));
    SyncBuiltinScoreOverride();
    PresentCustomFlyingScore(buffer, components);

    const auto hud = CutAccuracy::buildHudPresentation(sessionStats);
    if (traversal) {
        CutAccuracyLogger.info(
            "{} {:.1f}/{:.0f} | raw {:.2f}% level {:.2f}% | mult {}/{} | mini {:.1f}+{:.1f} swing {:.1f}+{:.1f} speed {:.1f} | traversal {:.1f}ms",
            rule.name, components.total(), rule.maxScore, sessionStats.averages().rawAccuracyPct, sessionStats.averages().levelAccuracyPct,
            ActualMultiplier(good), MaxMultiplier(good), components.firstMini, components.secondMini,
            components.beforeSwing, components.afterSwing, components.speed, *traversal * 1000.0);
    } else {
        CutAccuracyLogger.warn(
            "{} {:.1f}/{:.0f} | raw {:.2f}% level {:.2f}% | mult {}/{} | traversal -- (missing count {}) | {}",
            rule.name, components.total(), rule.maxScore, sessionStats.averages().rawAccuracyPct, sessionStats.averages().levelAccuracyPct,
            ActualMultiplier(good), MaxMultiplier(good), traversalMissingCount, hud.heading);
    }

    if (movement) preSwingDegrees.erase(movement);
    if (counter) postSwingDegrees.erase(counter);
    MarkHudDirty();
}

void OnScoringFinished(ScoringElement* element) {
    if (!element || !element->noteData) return;
    if (auto good = il2cpp_utils::try_cast<GoodCutScoringElement>(element)) {
        CommitGood(good.value());
    } else if (il2cpp_utils::try_cast<BadCutScoringElement>(element)) {
        CommitMiss(element->noteData, MaxMultiplier(element));
    } else if (il2cpp_utils::try_cast<MissScoringElement>(element)) {
        CommitMiss(element->noteData, MaxMultiplier(element));
    }
}

template<typename Hook>
bool TryInstallHook() {
    try {
        auto* info = Hook::getInfo();
        if (!info || !info->methodPointer) {
            CutAccuracyLogger.warn("Skipping hook {}: method not found", Hook::name());
            return false;
        }

        Hooking::__InstallHook<Hook>(CutAccuracyLogger, reinterpret_cast<void*>(info->methodPointer));
        return true;
    } catch (const std::exception& e) {
        CutAccuracyLogger.error("Skipping hook {}: {}", Hook::name(), e.what());
        return false;
    } catch (...) {
        CutAccuracyLogger.error("Skipping hook {}: unknown hook resolution failure", Hook::name());
        return false;
    }
}

} // namespace


MAKE_HOOK_MATCH(CA_GetNoteScoreDefinition,
    &ScoreModel::GetNoteScoreDefinition,
    ScoreModel_NoteScoreDefinition*, NoteData_ScoringType scoringType) {

    auto* def = CA_GetNoteScoreDefinition(scoringType);
    const auto rule = CutAccuracy::scoreObjectRuleForScoringType(static_cast<int>(scoringType));
    PatchScoreDefinition(def, rule);
    return def;
}

MAKE_HOOK_MATCH(CA_NoteWasCut,
    &BeatmapObjectManager::HandleNoteControllerNoteWasCut,
    void, BeatmapObjectManager* self, NoteController* noteController,
    ByRef<NoteCutInfo> noteCutInfo) {

    if (noteController && UsesCubeModel(noteController->noteData)) {
        auto noteTransform = noteController->get_noteTransform();
        auto* t = noteTransform.ptr();
        if (t) {
            const auto localPlane = WorldPlaneToLocal(t, noteCutInfo.heldRef.cutPoint, noteCutInfo.heldRef.cutNormal);
            const auto coreDirection = ToCoreCutDirection(noteController->noteData->cutDirection);
            const auto splitAxis = ResolveSplitAxisLocal(noteController->noteData, t, noteCutInfo.heldRef);

            if (CutAccuracy::lengthSq(splitAxis) > 1e-8) {
                const auto first = CutAccuracy::cutMiniNoteVolumes(splitAxis, true, localPlane);
                const auto second = CutAccuracy::cutMiniNoteVolumes(splitAxis, false, localPlane);

                pendingCuts[noteController->noteData] = {
                    ExpectedSide(noteController->noteData),
                    coreDirection,
                    CutAccuracy::smallerRatio(first),
                    CutAccuracy::smallerRatio(second),
                    FreezeBox(t),
                    static_cast<double>(UnityEngine::Time::get_time())
                };
            } else {
                CutAccuracyLogger.warn("No usable split axis for note; completed scoring will count it as zero");
            }
        }
    }

    CA_NoteWasCut(self, noteController, noteCutInfo);
}

MAKE_HOOK_MATCH(CA_ComputeSwingRating,
    static_cast<float (SaberMovementData::*)(bool, float)>(&SaberMovementData::ComputeSwingRating),
    float, SaberMovementData* self, bool overrideSegmentAngle, float overrideValue) {

    const float vanilla = CA_ComputeSwingRating(self, overrideSegmentAngle, overrideValue);
    if (!self || self->_validCount <= 0 || self->_data.size() == 0) return vanilla;

    const auto data = self->_data;
    const int length = data.size();
    int index = self->_nextAddIndex - 1;
    if (index < 0) index += length;

    const float startTime = data[index].time;
    float time = startTime;
    UnityEngine::Vector3 previousNormal = data[index].segmentNormal;
    double degrees = overrideSegmentAngle ? overrideValue : data[index].segmentAngle;

    for (int i = 2; startTime - time < 0.4f && i < self->_validCount && degrees < 60.0; ++i) {
        --index;
        if (index < 0) index += length;
        const auto& e = data[index];
        const float normalDiff = UnityEngine::Vector3::Angle(e.segmentNormal, previousNormal);
        if (normalDiff > 90.0f) break;
        degrees += e.segmentAngle;
        previousNormal = e.segmentNormal;
        time = e.time;
    }

    preSwingDegrees[self] = std::min(60.0, degrees);
    return vanilla;
}

MAKE_HOOK_MATCH(CA_ProcessNewSwingData,
    &SaberSwingRatingCounter::ProcessNewData,
    void, SaberSwingRatingCounter* self,
    BladeMovementDataElement newData,
    BladeMovementDataElement prevData,
    bool prevDataAreValid) {

    const bool wasPastPlane = self->_notePlaneWasCut;
    CA_ProcessNewSwingData(self, newData, prevData, prevDataAreValid);

    double& degrees = postSwingDegrees[self];
    if (degrees >= 60.0 || !prevDataAreValid) return;

    if (!wasPastPlane && self->_notePlaneWasCut) {
        const float partial = UnityEngine::Vector3::Angle(
            Subtract(self->_cutTopPos, self->_cutBottomPos),
            Subtract(self->_afterCutTopPos, self->_afterCutBottomPos));
        degrees = std::min(60.0, degrees + static_cast<double>(partial));
        return;
    }

    if (self->_notePlaneWasCut && self->_rateAfterCut) {
        const float normalDiff = UnityEngine::Vector3::Angle(newData.segmentNormal, self->_cutPlaneNormal);
        if (normalDiff <= 90.0f) {
            degrees = std::min(60.0, degrees + static_cast<double>(newData.segmentAngle));
        }
    }
}

MAKE_HOOK_MATCH(CA_ScoreControllerStart, &ScoreController::Start, void, ScoreController* self) {
    ClearHud();
    ClearFlyingScores();
    ResetSession();
    CA_ScoreControllerStart(self);

    // Defensive lifecycle handling: ScoreController normally starts once per play
    // scene. Avoid touching stale controller objects from a previous scene; Unity
    // can already be tearing them down when Continue starts the next level.
    if (scoreControllerWithDelegate && scoreControllerWithDelegate != self) {
        scoreControllerWithDelegate = nullptr;
        scoreFinishedDelegate = nullptr;
    } else if (scoreControllerWithDelegate && scoreFinishedDelegate) {
        scoreControllerWithDelegate->remove_scoringForNoteFinishedEvent(scoreFinishedDelegate);
    }

    scoreFinishedDelegate = custom_types::MakeDelegate<System::Action_1<ScoringElement*>*>(
        (std::function<void(ScoringElement*)>)OnScoringFinished);
    scoreControllerWithDelegate = self;
    if (self && scoreFinishedDelegate) {
        self->add_scoringForNoteFinishedEvent(scoreFinishedDelegate);
    }
}

MAKE_HOOK_MATCH(CA_ScoreControllerOnDestroy, &ScoreController::OnDestroy, void, ScoreController* self) {
    ClearHud();
    ClearFlyingScores();
    if (self && scoreFinishedDelegate && self == scoreControllerWithDelegate) {
        self->remove_scoringForNoteFinishedEvent(scoreFinishedDelegate);
        scoreFinishedDelegate = nullptr;
        scoreControllerWithDelegate = nullptr;
    }
    CA_ScoreControllerOnDestroy(self);
}


MAKE_HOOK_MATCH(CA_ComboUIControllerStart, &ComboUIController::Start, void, ComboUIController* self) {
    CA_ComboUIControllerStart(self);
    try {
        ClearHud();
        InstallHud(self);
    } catch (const std::exception& e) {
        CutAccuracyLogger.warn("CutAccuracy HUD install failed: {}", e.what());
        ClearHud();
    } catch (...) {
        CutAccuracyLogger.warn("CutAccuracy HUD install failed with an unknown exception");
        ClearHud();
    }
}

MAKE_HOOK_MATCH(CA_FlyingScoreInitAndPresent,
    &FlyingScoreEffect::InitAndPresent,
    void, FlyingScoreEffect* self, IReadonlyCutScoreBuffer* cutScoreBuffer,
    float duration, UnityEngine::Vector3 targetPos, UnityEngine::Color color) {

    CA_FlyingScoreInitAndPresent(self, cutScoreBuffer, duration, targetPos, color);
    RegisterFlyingScore(cutScoreBuffer, self);
}

MAKE_HOOK_CHECKED_FIND(CA_FlyingScoreDidFinish,
    &FlyingScoreEffect::HandleCutScoreBufferDidFinish,
    classof(FlyingScoreEffect*),
    "HandleCutScoreBufferDidFinish",
    void, FlyingScoreEffect* self, CutScoreBuffer* cutScoreBuffer) {

    CA_FlyingScoreDidFinish(self, cutScoreBuffer);
    if (cutScoreBuffer) {
        ReapplyCustomFlyingScore(cutScoreBuffer->i___GlobalNamespace__IReadonlyCutScoreBuffer());
    }
}

MAKE_HOOK_MATCH(CA_FlyingScoreRefreshScore,
    &FlyingScoreEffect::RefreshScore,
    void, FlyingScoreEffect* self, int score, int maxPossibleCutScore) {

    CA_FlyingScoreRefreshScore(self, score, maxPossibleCutScore);
    ReapplyCustomFlyingScore(self);
}

MAKE_HOOK_FIND_INSTANCE(CA_FlyingScoreUpdate,
    classof(FlyingScoreEffect*),
    "Update",
    void, FlyingScoreEffect* self) {

    CA_FlyingScoreUpdate(self);
    ReapplyCustomFlyingScore(self);
}

void InstallHooks() {
    int installed = 0;
    installed += TryInstallHook<Hook_CA_GetNoteScoreDefinition>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_NoteWasCut>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_ComputeSwingRating>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_ProcessNewSwingData>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_ScoreControllerStart>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_ScoreControllerOnDestroy>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_ComboUIControllerStart>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_FlyingScoreInitAndPresent>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_FlyingScoreDidFinish>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_FlyingScoreRefreshScore>() ? 1 : 0;
    installed += TryInstallHook<Hook_CA_FlyingScoreUpdate>() ? 1 : 0;

    CutAccuracyLogger.info("CutAccuracy installed {}/11 hooks", installed);
}

} // namespace CutAccuracyQuest
