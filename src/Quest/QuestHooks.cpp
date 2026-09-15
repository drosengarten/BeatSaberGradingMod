#include "main.hpp"
#include "Quest/QuestState.hpp"
#include "Quest/UnityAdapters.hpp"
#include "Quest/FloatingScoreView.hpp"
#include "Quest/Settings.hpp"
#include "CutAccuracy/Geometry.hpp"
#include "CutAccuracy/Scoring.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/delegate.hpp"
#include "metacore/shared/game.hpp"
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
#include "GlobalNamespace/FlyingScoreEffect.hpp"
#include "GlobalNamespace/IReadonlyCutScoreBuffer.hpp"
#include "GlobalNamespace/ScoringElement.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "System/Action_1.hpp"
#include "System/Action_2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Color.hpp"
#include "beatsaber-hook/shared/utils/byref.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
using namespace GlobalNamespace;
namespace CutAccuracyQuest { namespace {
System::Action_1<ScoringElement*>* scoreFinishedDelegate=nullptr;
ScoreController* scoreControllerWithDelegate=nullptr;
std::unordered_map<ScoringElement*,CutAccuracy::BeatSaberCutScoreParts> pendingScoreOverrides;
std::unordered_map<CutScoreBuffer*,CutAccuracy::BeatSaberCarrierDefinition> bufferCarriers;
bool runtimeScoringReady=false;
bool RuntimeCustomScoringActive(){return CustomScoringActive()&&runtimeScoringReady;}
struct NativeScoreDefinitionSnapshot {
    int maxCenterDistanceCutScore{};
    int minBeforeCutScore{};
    int maxBeforeCutScore{};
    int minAfterCutScore{};
    int maxAfterCutScore{};
    int fixedCutScore{};
};
std::unordered_map<ScoreModel_NoteScoreDefinition*, NativeScoreDefinitionSnapshot> nativeScoreDefinitions;

constexpr double kNativeBeforeFullDeg=100.0,kNativeAfterFullDeg=60.0,kSwingCaptureCapDeg=180.0;
UnityEngine::Vector3 Subtract(UnityEngine::Vector3 a,UnityEngine::Vector3 b){return{a.x-b.x,a.y-b.y,a.z-b.z};}
CutAccuracy::CutDirection ToCoreCutDirection(NoteCutDirection d){switch(static_cast<int>(d)){case 0:return CutAccuracy::CutDirection::Up;case 1:return CutAccuracy::CutDirection::Down;case 2:return CutAccuracy::CutDirection::Left;case 3:return CutAccuracy::CutDirection::Right;case 4:return CutAccuracy::CutDirection::UpLeft;case 5:return CutAccuracy::CutDirection::UpRight;case 6:return CutAccuracy::CutDirection::DownLeft;case 7:return CutAccuracy::CutDirection::DownRight;case 8:return CutAccuracy::CutDirection::Any;default:return CutAccuracy::CutDirection::None;}}
CutAccuracy::SaberSide ExpectedSide(NoteData*n){return static_cast<int>(n->colorType)==0?CutAccuracy::SaberSide::Left:CutAccuracy::SaberSide::Right;}
int ActualMultiplier(ScoringElement*e){return e?std::max(0,e->get_multiplier()):0;} int MaxMultiplier(ScoringElement*e){return e?std::max(1,e->get_maxMultiplier()):1;}
CutAccuracy::ProfileKind KindForNote(NoteData*n){if(!n)return CutAccuracy::ProfileKind::Excluded;int color=static_cast<int>(n->colorType);if(color!=0&&color!=1)return CutAccuracy::ProfileKind::Excluded;auto d=ToCoreCutDirection(n->cutDirection);return CutAccuracy::profileKindForScoringType(static_cast<int>(n->scoringType),d==CutAccuracy::CutDirection::Any);}
bool IsTracked(NoteData*n){return RuntimeCustomScoringActive()&&KindForNote(n)!=CutAccuracy::ProfileKind::Excluded;}
void RememberNativeDefinition(ScoreModel_NoteScoreDefinition* def) {
    if (!def || nativeScoreDefinitions.contains(def)) return;
    nativeScoreDefinitions.emplace(def, NativeScoreDefinitionSnapshot{
        def->___maxCenterDistanceCutScore,
        def->___minBeforeCutScore,
        def->___maxBeforeCutScore,
        def->___minAfterCutScore,
        def->___maxAfterCutScore,
        def->___fixedCutScore
    });
}

void RestoreNativeDefinition(ScoreModel_NoteScoreDefinition* def) {
    if (!def) return;
    const auto it = nativeScoreDefinitions.find(def);
    if (it == nativeScoreDefinitions.end()) return;
    const auto& n = it->second;
    def->___maxCenterDistanceCutScore = n.maxCenterDistanceCutScore;
    def->___minBeforeCutScore = n.minBeforeCutScore;
    def->___maxBeforeCutScore = n.maxBeforeCutScore;
    def->___minAfterCutScore = n.minAfterCutScore;
    def->___maxAfterCutScore = n.maxAfterCutScore;
    def->___fixedCutScore = n.fixedCutScore;
}

void ApplyCarrierDefinition(ScoreModel_NoteScoreDefinition* def, const CutAccuracy::BeatSaberCarrierDefinition& carrier) {
    if (!def) return;
    def->___maxCenterDistanceCutScore = carrier.centerDistanceMax;
    def->___minBeforeCutScore = 0;
    def->___maxBeforeCutScore = carrier.beforeMax;
    def->___minAfterCutScore = 0;
    def->___maxAfterCutScore = carrier.afterMax;
    def->___fixedCutScore = carrier.fixed;
}

void PatchScoreDefinition(ScoreModel_NoteScoreDefinition* def, int scoringType, bool dotNote = false) {
    if (!def) return;

    // Cache exact Beat Saber values before this mod ever mutates the shared object.
    // Off can then restore the actual 1.40.8 definition rather than guessing it.
    RememberNativeDefinition(def);
    if (!RuntimeCustomScoringActive()) {
        RestoreNativeDefinition(def);
        return;
    }

    const auto kind = CutAccuracy::profileKindForScoringType(scoringType, dotNote);
    if (kind == CutAccuracy::ProfileKind::Excluded) {
        // Unknown/no-score types are not part of Cut Accuracy. Preserve the
        // game's native definition instead of guessing that zero-score is safe.
        RestoreNativeDefinition(def);
        return;
    }

    // Chain links are natively fixed-score objects. If their custom profile is
    // also flat-only, keep the native definition so Beat Saber's chain-link
    // scoring lifecycle is left untouched. Only switch them to a measurement
    // carrier when the user explicitly enables Precise/Center/Before/After.
    const auto profile = CurrentProfile(kind);
    if (CutAccuracy::usesNativeFixedCarrier(kind, profile)) {
        ApplyCarrierDefinition(def, CutAccuracy::beatSaberFixedCarrier(profile.maxScore));
        return;
    }

    // Custom-scored objects still need Beat Saber's center/swing measurements,
    // but the carrier maximum must match the selected profile maximum. If it
    // stays at 115 while the profile max is 100, Beat Saber's level accuracy UI
    // can divide the custom score by the wrong denominator.
    ApplyCarrierDefinition(def, CutAccuracy::beatSaberMeasurementCarrier(profile.maxScore));
}

CutAccuracy::Vec3 ResolveSplitAxisLocal(NoteData*n,UnityEngine::Transform*t,const NoteCutInfo&info){if(!n||!t)return{0,0,0};auto d=ToCoreCutDirection(n->cutDirection);if(d==CutAccuracy::CutDirection::Any){auto local=LocalDirectionToNotePlane(t,info.saberDir);if(CutAccuracy::lengthSq(local)>1e-8){++dotNotesScored;return local;}return{0,0,0};}if(!CutAccuracy::hasDirectionalSplit(d))return{0,0,0};auto worldAxis=CutAccuracy::splitAxisForCutDirection(d,static_cast<double>(n->cutDirectionAngleOffset));auto localUpWorld=LocalNoteUpWorld(t);double aligned=CutAccuracy::dot(localUpWorld,CutAccuracy::normalized(worldAxis));if(aligned>0.75)return{0,1,0};if(aligned<-0.75)return{0,-1,0};auto local=WorldDirectionToLocalNotePlane(t,worldAxis);return CutAccuracy::lengthSq(local)>1e-8?local:CutAccuracy::Vec3{0,1,0};}
DepthSplitMiniRatios Ratios(const CutAccuracy::DepthSplitMiniNoteVolumes&v){return{CutAccuracy::smallerRatio(v.negativeDepth),CutAccuracy::smallerRatio(v.positiveDepth)};}
CutAccuracy::FourMiniNoteQuality FourQuality(const PendingCut&p){return{CutAccuracy::miniQualityFromSmallerRatio(p.upperRatios.negativeDepth),CutAccuracy::miniQualityFromSmallerRatio(p.upperRatios.positiveDepth),CutAccuracy::miniQualityFromSmallerRatio(p.lowerRatios.negativeDepth),CutAccuracy::miniQualityFromSmallerRatio(p.lowerRatios.positiveDepth)};}
CutAccuracy::BeatSaberCarrierDefinition CarrierForDefinition(ScoreModel_NoteScoreDefinition*def){return def?CutAccuracy::BeatSaberCarrierDefinition{def->___maxCenterDistanceCutScore,def->___maxBeforeCutScore,def->___maxAfterCutScore,def->___fixedCutScore}:CutAccuracy::beatSaberMeasurementCarrier(100);}
CutAccuracy::BeatSaberCarrierDefinition CarrierForProfile(CutAccuracy::ProfileKind kind,const CutAccuracy::ScoringProfile&profile){return CutAccuracy::usesNativeFixedCarrier(kind,profile)?CutAccuracy::beatSaberFixedCarrier(profile.maxScore):CutAccuracy::beatSaberMeasurementCarrier(profile.maxScore);}
CutAccuracy::BeatSaberCarrierDefinition CarrierForBuffer(CutScoreBuffer*b){if(!b)return CutAccuracy::beatSaberMeasurementCarrier(100);auto it=bufferCarriers.find(b);if(it!=bufferCarriers.end())return it->second;return CarrierForDefinition(b->_noteScoreDefinition);}
double CenterQuality(CutScoreBuffer*b){auto c=CarrierForBuffer(b);return b&&c.centerDistanceMax>0?std::clamp(static_cast<double>(b->_centerDistanceCutScore)/static_cast<double>(c.centerDistanceMax),0.0,1.0):0.0;}
void ApplyScoreParts(CutScoreBuffer*b,const CutAccuracy::BeatSaberCutScoreParts&p){if(!b)return;b->_centerDistanceCutScore=p.centerDistance;b->_beforeCutScore=p.before;b->_afterCutScore=p.after;}
void ApplyPendingScoreOverride(ScoringElement*e){if(!e)return;auto it=pendingScoreOverrides.find(e);if(it==pendingScoreOverrides.end())return;if(auto good=il2cpp_utils::try_cast<GoodCutScoringElement>(e))ApplyScoreParts(good.value()->_cutScoreBuffer,it->second);}
void SyncBuiltinScoreOverride(){if(!RuntimeCustomScoringActive()||!scoreControllerWithDelegate)return;int max=CutAccuracy::customInternalMaxScore(sessionStats.levelMax());int score=CutAccuracy::customInternalScoreFromCustomLevel(sessionStats.levelEarned(),sessionStats.levelMax());float modifier=scoreControllerWithDelegate->_prevMultiplierFromModifiers;int modified=ScoreModel::GetModifiedScoreForGameplayModifiersScoreMultiplier(score,modifier);int modifiedMax=ScoreModel::GetModifiedScoreForGameplayModifiersScoreMultiplier(max,modifier);scoreControllerWithDelegate->_multipliedScore=score;scoreControllerWithDelegate->_immediateMaxPossibleMultipliedScore=max;scoreControllerWithDelegate->_modifiedScore=modified;scoreControllerWithDelegate->_immediateMaxPossibleModifiedScore=modifiedMax;if(scoreControllerWithDelegate->scoreDidChangeEvent)scoreControllerWithDelegate->scoreDidChangeEvent->Invoke(score,modified);lastBuiltinScoreOverride=score;lastBuiltinMaxScoreObserved=max;++builtinScoreOverridesApplied;}
void CommitFailure(ScoringElement*element,bool badCut){if(!element||!element->noteData)return;auto*n=element->noteData;auto k=KindForNote(n);if(k==CutAccuracy::ProfileKind::Excluded){++objectsIgnored;pendingCuts.erase(n);return;}auto p=CurrentProfile(k);double score=CutAccuracy::failureScore(badCut?p.badCut:p.miss,p.maxScore);sessionStats.forSide(ExpectedSide(n)).addFixed(score,p.maxScore,ActualMultiplier(element),MaxMultiplier(element));pendingCuts.erase(n);SyncBuiltinScoreOverride();}
void CommitGood(GoodCutScoringElement*good){if(!good||!good->noteData)return;auto*n=good->noteData;auto k=KindForNote(n);if(k==CutAccuracy::ProfileKind::Excluded){++objectsIgnored;pendingCuts.erase(n);return;}auto profile=CurrentProfile(k);auto*b=good->_cutScoreBuffer;CutAccuracy::ScoringInput input{};auto it=pendingCuts.find(n);CutAccuracy::SaberSide side=ExpectedSide(n);if(it!=pendingCuts.end()){side=it->second.side;input.mini=FourQuality(it->second);input.preciseAvailable=true;pendingCuts.erase(it);} // Read native center accuracy BEFORE replacing native buckets.
 if(b)input.centerQuality=CenterQuality(b);auto*counter=b?b->_saberSwingRatingCounter:nullptr;auto*movement=counter?(SaberMovementData*)counter->_saberMovementData:nullptr;if(movement&&preSwingDegrees.contains(movement))input.beforeSwingDeg=preSwingDegrees[movement];else if(b&&counter)input.beforeSwingDeg=std::clamp(static_cast<double>(b->get_beforeCutSwingRating()),0.0,1.0)*kNativeBeforeFullDeg;if(counter&&postSwingDegrees.contains(counter))input.afterSwingDeg=postSwingDegrees[counter];else if(b&&counter)input.afterSwingDeg=std::clamp(static_cast<double>(b->get_afterCutSwingRating()),0.0,1.0)*kNativeAfterFullDeg;auto scored=CutAccuracy::scoreValidCut(input,profile);const bool nativeFixedCarrier=CutAccuracy::usesNativeFixedCarrier(k,profile);if(!nativeFixedCarrier){auto parts=CutAccuracy::beatSaberCutScoreParts(scored,CarrierForBuffer(b));pendingScoreOverrides[good]=parts;ApplyScoreParts(b,parts);}sessionStats.forSide(side).addScored(scored,ActualMultiplier(good),MaxMultiplier(good));SyncBuiltinScoreOverride();if(ShouldShowFlyingScore())PresentCustomFlyingScore(b,scored);if(k==CutAccuracy::ProfileKind::ChainLink||k==CutAccuracy::ProfileKind::ChainLinkArcHead)++chainLinksScored;CutAccuracyLogger.info("{} {:.1f}/{:.1f} | precise {:.1f}% (upper {:.1f}% lower {:.1f}%) center {:.1f}% before {:.1f}% after {:.1f}%",std::string(CutAccuracy::profileKindName(k)),scored.totalScore,scored.maxScore,scored.preciseQuality*100.0,scored.upperPairQuality*100.0,scored.lowerPairQuality*100.0,scored.centerQuality*100.0,scored.beforeQuality*100.0,scored.afterQuality*100.0);if(movement)preSwingDegrees.erase(movement);if(counter)postSwingDegrees.erase(counter);}
void OnScoringFinished(ScoringElement*e){if(!RuntimeCustomScoringActive()||!e||!e->noteData)return;if(auto good=il2cpp_utils::try_cast<GoodCutScoringElement>(e))CommitGood(good.value());else if(il2cpp_utils::try_cast<BadCutScoringElement>(e))CommitFailure(e,true);else if(il2cpp_utils::try_cast<MissScoringElement>(e))CommitFailure(e,false);}
template<typename Hook> bool TryInstallHook(){try{auto*info=Hook::getInfo();if(!info||!info->methodPointer){CutAccuracyLogger.warn("Skipping hook {}: method not found",Hook::name());return false;}Hooking::__InstallHook<Hook>(CutAccuracyLogger,reinterpret_cast<void*>(info->methodPointer));return true;}catch(...){CutAccuracyLogger.warn("Skipping hook {}",Hook::name());return false;}}
}
MAKE_HOOK_MATCH(CA_GetNoteScoreDefinition,&ScoreModel::GetNoteScoreDefinition,ScoreModel_NoteScoreDefinition*,NoteData_ScoringType scoringType){auto*def=CA_GetNoteScoreDefinition(scoringType);PatchScoreDefinition(def,static_cast<int>(scoringType));return def;}
MAKE_HOOK_MATCH(CA_CutScoreBufferInit,&CutScoreBuffer::Init,bool,CutScoreBuffer*self,ByRef<NoteCutInfo>noteCutInfo){auto ok=CA_CutScoreBufferInit(self,noteCutInfo);if(RuntimeCustomScoringActive()&&self&&noteCutInfo.heldRef.noteData){auto d=ToCoreCutDirection(noteCutInfo.heldRef.noteData->cutDirection);auto kind=CutAccuracy::profileKindForScoringType(static_cast<int>(noteCutInfo.heldRef.noteData->scoringType),d==CutAccuracy::CutDirection::Any);PatchScoreDefinition(self->_noteScoreDefinition,static_cast<int>(noteCutInfo.heldRef.noteData->scoringType),d==CutAccuracy::CutDirection::Any);if(kind!=CutAccuracy::ProfileKind::Excluded)bufferCarriers[self]=CarrierForProfile(kind,CurrentProfile(kind));else bufferCarriers.erase(self);}return ok;}
MAKE_HOOK_MATCH(CA_NoteWasCut,&BeatmapObjectManager::HandleNoteControllerNoteWasCut,void,BeatmapObjectManager*self,NoteController*noteController,ByRef<NoteCutInfo>noteCutInfo){if(noteController&&IsTracked(noteController->noteData)){auto kind=KindForNote(noteController->noteData);auto profile=CurrentProfile(kind);if(profile.precise.enabled){auto nt=noteController->get_noteTransform();auto*t=nt.ptr();if(t){auto plane=WorldPlaneToLocal(t,noteCutInfo.heldRef.cutPoint,noteCutInfo.heldRef.cutNormal);auto d=ToCoreCutDirection(noteController->noteData->cutDirection);auto axis=ResolveSplitAxisLocal(noteController->noteData,t,noteCutInfo.heldRef);if(CutAccuracy::lengthSq(axis)>1e-8){auto upper=CutAccuracy::cutDepthSplitMiniNoteVolumes(axis,true,plane);auto lower=CutAccuracy::cutDepthSplitMiniNoteVolumes(axis,false,plane);pendingCuts[noteController->noteData]={ExpectedSide(noteController->noteData),d,Ratios(upper),Ratios(lower)};}}}}CA_NoteWasCut(self,noteController,noteCutInfo);}
MAKE_HOOK_MATCH(CA_ComputeSwingRating,static_cast<float(SaberMovementData::*)(bool,float)>(&SaberMovementData::ComputeSwingRating),float,SaberMovementData*self,bool overrideSegmentAngle,float overrideValue){float vanilla=CA_ComputeSwingRating(self,overrideSegmentAngle,overrideValue);if(!RuntimeCustomScoringActive()||!self||self->_validCount<=0||self->_data.size()==0)return vanilla;auto data=self->_data;int len=data.size(),index=self->_nextAddIndex-1;if(index<0)index+=len;float start=data[index].time,time=start;auto prev=data[index].segmentNormal;double degrees=overrideSegmentAngle?overrideValue:data[index].segmentAngle;for(int i=2;start-time<0.4f&&i<self->_validCount&&degrees<kSwingCaptureCapDeg;++i){--index;if(index<0)index+=len;auto&e=data[index];if(UnityEngine::Vector3::Angle(e.segmentNormal,prev)>90)break;degrees+=e.segmentAngle;prev=e.segmentNormal;time=e.time;}preSwingDegrees[self]=std::min(kSwingCaptureCapDeg,degrees);return vanilla;}
MAKE_HOOK_MATCH(CA_ProcessNewSwingData,&SaberSwingRatingCounter::ProcessNewData,void,SaberSwingRatingCounter*self,BladeMovementDataElement newData,BladeMovementDataElement prevData,bool prevValid){bool was=self->_notePlaneWasCut;CA_ProcessNewSwingData(self,newData,prevData,prevValid);if(!RuntimeCustomScoringActive()||!self)return;double&deg=postSwingDegrees[self];if(deg>=kSwingCaptureCapDeg||!prevValid)return;if(!was&&self->_notePlaneWasCut){float partial=UnityEngine::Vector3::Angle(Subtract(self->_cutTopPos,self->_cutBottomPos),Subtract(self->_afterCutTopPos,self->_afterCutBottomPos));deg=std::min(kSwingCaptureCapDeg,deg+static_cast<double>(partial));return;}if(self->_notePlaneWasCut&&self->_rateAfterCut){float nd=UnityEngine::Vector3::Angle(newData.segmentNormal,self->_cutPlaneNormal);if(nd<=90)deg=std::min(kSwingCaptureCapDeg,deg+static_cast<double>(newData.segmentAngle));}}
MAKE_HOOK_MATCH(CA_ScoreControllerLateUpdate,&ScoreController::LateUpdate,void,ScoreController*self){CA_ScoreControllerLateUpdate(self);if(self==scoreControllerWithDelegate)SyncBuiltinScoreOverride();}
MAKE_HOOK_MATCH(CA_ScoreControllerStart,&ScoreController::Start,void,ScoreController*self){
    ClearFlyingScores();pendingScoreOverrides.clear();bufferCarriers.clear();ResetSession();
    CA_ScoreControllerStart(self);
    if(scoreControllerWithDelegate&&scoreControllerWithDelegate!=self){scoreControllerWithDelegate=nullptr;scoreFinishedDelegate=nullptr;}
    else if(scoreControllerWithDelegate&&scoreFinishedDelegate)scoreControllerWithDelegate->remove_scoringForNoteFinishedEvent(scoreFinishedDelegate);
    if(!CustomScoringActive())return;
    if(!runtimeScoringReady){
        CutAccuracyLogger.error("CutAccuracy custom mode requested, but required scoring hooks are unavailable; leaving Beat Saber scoring untouched");
        return;
    }
    scoreFinishedDelegate=custom_types::MakeDelegate<System::Action_1<ScoringElement*>*>((std::function<void(ScoringElement*)>)OnScoringFinished);
    scoreControllerWithDelegate=self;
    if(self&&scoreFinishedDelegate)self->add_scoringForNoteFinishedEvent(scoreFinishedDelegate);
    CutAccuracyLogger.info("CutAccuracy custom scoring active in mode {}; external score submission disabled while custom mode is selected",static_cast<int>(CurrentScoringMode()));
}
MAKE_HOOK_MATCH(CA_ScoreControllerOnDestroy,&ScoreController::OnDestroy,void,ScoreController*self){ClearFlyingScores();pendingScoreOverrides.clear();bufferCarriers.clear();if(self&&scoreFinishedDelegate&&self==scoreControllerWithDelegate){self->remove_scoringForNoteFinishedEvent(scoreFinishedDelegate);scoreFinishedDelegate=nullptr;scoreControllerWithDelegate=nullptr;}CA_ScoreControllerOnDestroy(self);}
MAKE_HOOK_MATCH(CA_ScoreControllerDespawnScoringElement,&ScoreController::DespawnScoringElement,void,ScoreController*self,ScoringElement*e){CutScoreBuffer*b=nullptr;if(e){auto good=il2cpp_utils::try_cast<GoodCutScoringElement>(e);if(good)b=good.value()->_cutScoreBuffer;}if(RuntimeCustomScoringActive())ApplyPendingScoreOverride(e);CA_ScoreControllerDespawnScoringElement(self,e);if(e)pendingScoreOverrides.erase(e);if(b)bufferCarriers.erase(b);}
MAKE_HOOK_MATCH(CA_FlyingScoreInitAndPresent,&FlyingScoreEffect::InitAndPresent,void,FlyingScoreEffect*self,IReadonlyCutScoreBuffer*b,float duration,UnityEngine::Vector3 pos,UnityEngine::Color color){CA_FlyingScoreInitAndPresent(self,b,duration,pos,color);if(RuntimeCustomScoringActive()&&ShouldShowFlyingScore())RegisterFlyingScore(b,self);}
MAKE_HOOK_CHECKED_FIND(CA_FlyingScoreDidFinish,&FlyingScoreEffect::HandleCutScoreBufferDidFinish,classof(FlyingScoreEffect*),"HandleCutScoreBufferDidFinish",void,FlyingScoreEffect*self,CutScoreBuffer*b){CA_FlyingScoreDidFinish(self,b);if(RuntimeCustomScoringActive()&&ShouldShowFlyingScore()&&b)ReapplyCustomFlyingScore(b->i___GlobalNamespace__IReadonlyCutScoreBuffer());}
MAKE_HOOK_MATCH(CA_FlyingScoreRefreshScore,&FlyingScoreEffect::RefreshScore,void,FlyingScoreEffect*self,int score,int maxPossibleCutScore){CA_FlyingScoreRefreshScore(self,score,maxPossibleCutScore);if(RuntimeCustomScoringActive()&&ShouldShowFlyingScore())ReapplyCustomFlyingScore(self);}
MAKE_HOOK_FIND_INSTANCE(CA_FlyingScoreUpdate,classof(FlyingScoreEffect*),"Update",void,FlyingScoreEffect*self){CA_FlyingScoreUpdate(self);if(RuntimeCustomScoringActive()&&ShouldShowFlyingScore())ReapplyCustomFlyingScore(self);}
bool RuntimeScoringReady(){return runtimeScoringReady;}

void UpdateScoreSubmissionPolicy(){
    try{
        MetaCore::Game::SetScoreSubmission(MOD_ID,CutAccuracy::scoreSubmissionAllowedForMode(CurrentScoringMode()));
    }catch(const std::exception& e){
        CutAccuracyLogger.error("CutAccuracy could not update score-submission policy: {}",e.what());
    }catch(...){
        CutAccuracyLogger.error("CutAccuracy could not update score-submission policy");
    }
}

void InstallHooks(){
    int n=0;
    const bool scoreDefinition=TryInstallHook<Hook_CA_GetNoteScoreDefinition>(); n+=scoreDefinition;
    const bool scoreBufferInit=TryInstallHook<Hook_CA_CutScoreBufferInit>(); n+=scoreBufferInit;
    const bool preciseGeometry=TryInstallHook<Hook_CA_NoteWasCut>(); n+=preciseGeometry;
    const bool beforeSwing=TryInstallHook<Hook_CA_ComputeSwingRating>(); n+=beforeSwing;
    const bool afterSwing=TryInstallHook<Hook_CA_ProcessNewSwingData>(); n+=afterSwing;
    const bool scoreLifecycle=TryInstallHook<Hook_CA_ScoreControllerStart>(); n+=scoreLifecycle;
    const bool scoreCleanup=TryInstallHook<Hook_CA_ScoreControllerOnDestroy>(); n+=scoreCleanup;
    const bool scoreDespawn=TryInstallHook<Hook_CA_ScoreControllerDespawnScoringElement>(); n+=scoreDespawn;
    const bool scoreLateUpdate=TryInstallHook<Hook_CA_ScoreControllerLateUpdate>(); n+=scoreLateUpdate;
    n+=TryInstallHook<Hook_CA_FlyingScoreInitAndPresent>();
    n+=TryInstallHook<Hook_CA_FlyingScoreDidFinish>();
    n+=TryInstallHook<Hook_CA_FlyingScoreRefreshScore>();
    n+=TryInstallHook<Hook_CA_FlyingScoreUpdate>();
    runtimeScoringReady=scoreDefinition&&scoreBufferInit&&preciseGeometry&&beforeSwing&&afterSwing&&scoreLifecycle&&scoreCleanup&&scoreDespawn&&scoreLateUpdate;
    CutAccuracyLogger.info("CutAccuracy installed {}/13 hooks; core scoring hooks {}",n,runtimeScoringReady?"ready":"INCOMPLETE");
    if(!runtimeScoringReady)CutAccuracyLogger.error("CutAccuracy fail-safe engaged: custom scoring will not run because one or more required hooks are unavailable");
    UpdateScoreSubmissionPolicy();
}
}
