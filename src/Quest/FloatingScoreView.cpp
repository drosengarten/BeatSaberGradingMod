#include "Quest/FloatingScoreView.hpp"
#include "Quest/Settings.hpp"
#include "CutAccuracy/Presentation.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/FlyingScoreEffect.hpp"
#include "GlobalNamespace/IReadonlyCutScoreBuffer.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "TMPro/TextMeshPro.hpp"
#include "UnityEngine/Color.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
namespace CutAccuracyQuest{namespace{
struct State{GlobalNamespace::FlyingScoreEffect*effect{};std::string score,text;double pct{};};
std::unordered_map<GlobalNamespace::IReadonlyCutScoreBuffer*,State> states;
void Apply(GlobalNamespace::IReadonlyCutScoreBuffer*b){auto it=states.find(b);if(it==states.end())return;auto&s=it->second;if(!s.effect||s.score.empty()||!s.effect->_text)return;std::string text=s.score;if(ShouldShowFlyingScoreText()&&!s.text.empty())text+="\n<size=60%>"+s.text+"</size>";auto hex=CutAccuracy::accuracyBandHex(s.pct);s.effect->_text->set_richText(true);s.effect->_text->set_alignment(TMPro::TextAlignmentOptions::Center);s.effect->_text->set_text("<color="+std::string(hex)+">"+text+"</color>");auto rgb=CutAccuracy::accuracyBandRgb(s.pct);s.effect->_text->set_color({rgb.r,rgb.g,rgb.b,1});}
}
void RegisterFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer*b,GlobalNamespace::FlyingScoreEffect*e){if(!b||!e)return;for(auto it=states.begin();it!=states.end();)if(it->second.effect==e)it=states.erase(it);else ++it;states[b].effect=e;Apply(b);}
void PresentCustomFlyingScore(GlobalNamespace::CutScoreBuffer*b,const CutAccuracy::ScoredNote&s){if(!b)return;auto*r=b->i___GlobalNamespace__IReadonlyCutScoreBuffer();states[r].score=CutAccuracy::formatPerNoteScore(s);states[r].pct=s.normalizedScore()*100.0;states[r].text=FlyingScoreTextForAccuracy(states[r].pct);Apply(r);}
void PresentFixedFlyingScore(GlobalNamespace::CutScoreBuffer*b,double score,double max){if(!b)return;auto*r=b->i___GlobalNamespace__IReadonlyCutScoreBuffer();states[r].score=CutAccuracy::formatFixedScore(score,max);states[r].pct=max>0?std::clamp(score/max*100.0,0.0,100.0):0;states[r].text=FlyingScoreTextForAccuracy(states[r].pct);Apply(r);}
void ReapplyCustomFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer*b){Apply(b);}void ReapplyCustomFlyingScore(GlobalNamespace::FlyingScoreEffect*e){if(e&&e->_cutScoreBuffer)Apply(e->_cutScoreBuffer);}void ClearFlyingScores(){states.clear();}
}
