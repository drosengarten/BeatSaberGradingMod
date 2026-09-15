#include "Quest/Settings.hpp"
#include "main.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/InputFieldView.hpp"
#include "HMUI/TextSegmentedControl.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "TMPro/FontStyles.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "config-utils/shared/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <memory>
#include <optional>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace CutAccuracyQuest { namespace {
using namespace CutAccuracy;
DECLARE_CONFIG(CutAccuracyConfig) {
    CONFIG_VALUE(Mode, int, "Mode", 2, "0 Off, 1 Simple, 2 Advanced");
    // Keep legacy key so an existing 0.12.2 install migrates its accuracy weighting.
    CONFIG_VALUE(AccuracyWeightPct, int, "AccuracyWeightPct", 13, "Simple accuracy weight");
    CONFIG_VALUE(SimpleWeightsInitialized, bool, "SimpleWeightsInitialized", false, "Migration marker");
    CONFIG_VALUE(SimpleAccuracyMethod, int, "SimpleAccuracyMethod", 1, "0 Beat Saber, 1 Precise");
    CONFIG_VALUE(SimpleFullNoteMax, int, "SimpleFullNoteMax", 100, "Simple full-note maximum");
    CONFIG_VALUE(SimpleChainLinkMax, int, "SimpleChainLinkMax", 20, "Simple chain-link maximum");
    CONFIG_VALUE(SimpleBeforeWeightPct, int, "SimpleBeforeWeightPct", 61, "Simple before-swing weight");
    CONFIG_VALUE(SimpleAfterWeightPct, int, "SimpleAfterWeightPct", 26, "Simple after-swing weight");
    CONFIG_VALUE(SimpleBeforeAngle, int, "SimpleBeforeAngle", 100, "Simple before full-credit angle");
    CONFIG_VALUE(SimpleAfterAngle, int, "SimpleAfterAngle", 60, "Simple after full-credit angle");
    CONFIG_VALUE(SimpleUpperPairWeightPct, int, "SimpleUpperPairWeightPct", 50, "Precise Upper-pair share");
    CONFIG_VALUE(ShowFlyingScore, bool, "ShowFlyingScore", true, "Show custom flying cut scores");

    CONFIG_VALUE(AdvDirectional, std::string, "AdvDirectional", "");
    CONFIG_VALUE(AdvDot, std::string, "AdvDot", "");
    CONFIG_VALUE(AdvArcHead, std::string, "AdvArcHead", "");
    CONFIG_VALUE(AdvArcTail, std::string, "AdvArcTail", "");
    CONFIG_VALUE(AdvChainHead, std::string, "AdvChainHead", "");
    CONFIG_VALUE(AdvChainLink, std::string, "AdvChainLink", "");
    CONFIG_VALUE(AdvArcHeadArcTail, std::string, "AdvArcHeadArcTail", "");
    CONFIG_VALUE(AdvChainHeadArcTail, std::string, "AdvChainHeadArcTail", "");
    CONFIG_VALUE(AdvChainHeadArcHead, std::string, "AdvChainHeadArcHead", "");
    CONFIG_VALUE(AdvChainHeadArcHeadArcTail, std::string, "AdvChainHeadArcHeadArcTail", "");
    CONFIG_VALUE(AdvChainLinkArcHead, std::string, "AdvChainLinkArcHead", "");

    CONFIG_VALUE(ShowScoreText, bool, "ShowScoreText", true, "Show custom captions under flying cut scores");
    CONFIG_VALUE(EditScoreTextLabels, bool, "EditScoreTextLabels", false, "Expand custom caption label editor");
    CONFIG_VALUE(ScoreText0, std::string, "ScoreText0", "Dire");
    CONFIG_VALUE(ScoreText10, std::string, "ScoreText10", "Grim");
    CONFIG_VALUE(ScoreText20, std::string, "ScoreText20", "Rough");
    CONFIG_VALUE(ScoreText30, std::string, "ScoreText30", "Weak");
    CONFIG_VALUE(ScoreText40, std::string, "ScoreText40", "Messy");
    CONFIG_VALUE(ScoreText50, std::string, "ScoreText50", "Scrappy");
    CONFIG_VALUE(ScoreText60, std::string, "ScoreText60", "Decent");
    CONFIG_VALUE(ScoreText70, std::string, "ScoreText70", "Solid");
    CONFIG_VALUE(ScoreText80, std::string, "ScoreText80", "Sharp");
    CONFIG_VALUE(ScoreText90, std::string, "ScoreText90", "Excellent");
    CONFIG_VALUE(ScoreText100, std::string, "ScoreText100", "Perfect");
};

AdvancedConfig advancedRuntime{};

ConfigUtils::ConfigValue<std::string>& ProfileValue(ProfileKind kind) {
    auto& c=getCutAccuracyConfig();
    switch(kind){
        case ProfileKind::DirectionalNote:return c.AdvDirectional;
        case ProfileKind::DotNote:return c.AdvDot;
        case ProfileKind::ArcHead:return c.AdvArcHead;
        case ProfileKind::ArcTail:return c.AdvArcTail;
        case ProfileKind::ChainHead:return c.AdvChainHead;
        case ProfileKind::ChainLink:return c.AdvChainLink;
        case ProfileKind::ArcHeadArcTail:return c.AdvArcHeadArcTail;
        case ProfileKind::ChainHeadArcTail:return c.AdvChainHeadArcTail;
        case ProfileKind::ChainHeadArcHead:return c.AdvChainHeadArcHead;
        case ProfileKind::ChainHeadArcHeadArcTail:return c.AdvChainHeadArcHeadArcTail;
        case ProfileKind::ChainLinkArcHead:return c.AdvChainLinkArcHead;
        default:return c.AdvDirectional;
    }
}

void CanonicalizeProfile(ScoringProfile& p);
void EnforceAccuracyMethod(ScoringProfile& p);

std::string Serialize(const ScoringProfile&p){
    // v4: field 2 is Flat weight %, not absolute points.
    std::ostringstream o; o<<"5|"<<p.maxScore<<'|'<<p.flat.weightPct<<'|'<<p.precise.enabled<<'|'<<p.precise.weightPct<<'|'<<p.precise.upperPairWeightPct<<'|'<<p.center.enabled<<'|'<<p.center.weightPct<<'|'<<p.before.enabled<<'|'<<p.before.weightPct<<'|'<<p.before.fullCreditAngleDeg<<'|'<<p.after.enabled<<'|'<<p.after.weightPct<<'|'<<p.after.fullCreditAngleDeg<<'|'<<static_cast<int>(p.badCut.mode)<<'|'<<p.badCut.value<<'|'<<static_cast<int>(p.miss.mode)<<'|'<<p.miss.value<<'|'<<static_cast<int>(p.accuracyMethod)<<'|'<<p.preciseMixPct; return o.str();
}

bool Parse(const std::string&s,ScoringProfile& p){
    if(s.empty())return false; std::vector<std::string>v;std::stringstream ss(s);std::string x;while(std::getline(ss,x,'|'))v.push_back(x);
    if(!((v.size()==18&&(v[0]=="1"||v[0]=="2"||v[0]=="3"||v[0]=="4"))||(v.size()==20&&v[0]=="5")))return false;
    try{
        const bool v4=v[0]=="4"||v[0]=="5";
        p.maxScore=clampMaxScore(std::stod(v[1]));
        const double oldFlatOrWeight=std::stod(v[2]);
        p.precise.enabled=std::stoi(v[3])!=0;p.precise.weightPct=roundedClampedToInt(std::stod(v[4]),0,100);p.precise.upperPairWeightPct=std::clamp(std::stoi(v[5]),0,100);
        p.center.enabled=std::stoi(v[6])!=0;p.center.weightPct=roundedClampedToInt(std::stod(v[7]),0,100);
        p.before.enabled=std::stoi(v[8])!=0;p.before.weightPct=roundedClampedToInt(std::stod(v[9]),0,100);p.before.fullCreditAngleDeg=std::stod(v[10]);
        p.after.enabled=std::stoi(v[11])!=0;p.after.weightPct=roundedClampedToInt(std::stod(v[12]),0,100);p.after.fullCreditAngleDeg=std::stod(v[13]);
        p.badCut.mode=static_cast<FailureMode>(std::clamp(std::stoi(v[14]),0,2));p.badCut.value=std::stod(v[15]);p.miss.mode=static_cast<FailureMode>(std::clamp(std::stoi(v[16]),0,2));p.miss.value=std::stod(v[17]);
        if(v4){
            p.flat.weightPct=roundedClampedToInt(oldFlatOrWeight,0,100);
        }else{
            // v1-v3 stored Flat as absolute points and all other weights as shares
            // of the remaining pool. Convert them into direct shares of maxScore.
            const double oldFlat=std::clamp(oldFlatOrWeight,0.0,p.maxScore);
            const double variableFraction=p.maxScore>1e-9?(p.maxScore-oldFlat)/p.maxScore:0.0;
            p.flat.weightPct=p.maxScore>1e-9?roundedClampedToInt(oldFlat/p.maxScore*100.0,0,100):0;
            auto convert=[&](WeightedComponent& c){if(c.enabled)c.weightPct=roundedClampedToInt(static_cast<double>(c.weightPct)*variableFraction,0,100);};
            convert(p.precise);convert(p.center);convert(p.before);convert(p.after);
        }
        p.flat.enabled=p.flat.weightPct>0;
        CanonicalizeProfile(p);
        if(v[0]=="5") {
            p.accuracyMethod=static_cast<ProfileAccuracyMethod>(std::clamp(std::stoi(v[18]),0,2));
            p.preciseMixPct=clampPercent(std::stoi(v[19]));
        } else {
            p.accuracyMethod=p.precise.weightPct>0 ? (p.center.weightPct>0 ? ProfileAccuracyMethod::Blended : ProfileAccuracyMethod::Precise) : ProfileAccuracyMethod::BeatSaber;
            const int total=accuracyWeight(p);
            p.preciseMixPct=total>0 ? static_cast<int>(std::lround(100.0*p.precise.weightPct/total)) : 50;
        }
        EnforceAccuracyMethod(p);
        CanonicalizeProfile(p);
        return true;
    }catch(...){return false;}
}

void CanonicalizeProfile(ScoringProfile& p){
    p.maxScore=static_cast<double>(roundedClampedToInt(p.maxScore,0,1000));
    p.precise.upperPairWeightPct=clampPercent(p.precise.upperPairWeightPct);
    p.preciseMixPct=clampPercent(p.preciseMixPct);
    p.before.fullCreditAngleDeg=static_cast<double>(roundedClampedToInt(p.before.fullCreditAngleDeg,1,180));
    p.after.fullCreditAngleDeg=static_cast<double>(roundedClampedToInt(p.after.fullCreditAngleDeg,1,180));
    normalizeEnabledWeights(p);
    normalizeFailureRule(p.badCut,p.maxScore);
    normalizeFailureRule(p.miss,p.maxScore);
    if(p.badCut.mode==FailureMode::FixedPoints)p.badCut.value=static_cast<double>(roundedClampedToInt(p.badCut.value,0,roundedClampedToInt(p.maxScore,0,1000)));
    if(p.miss.mode==FailureMode::FixedPoints)p.miss.value=static_cast<double>(roundedClampedToInt(p.miss.value,0,roundedClampedToInt(p.maxScore,0,1000)));
}

void EnforceAccuracyMethod(ScoringProfile& p){
    p.accuracyMethod=static_cast<ProfileAccuracyMethod>(std::clamp(static_cast<int>(p.accuracyMethod),0,2));
    p.preciseMixPct=clampPercent(p.preciseMixPct);
    const int total=accuracyWeight(p);
    setAccuracyWeight(p,total);
}

void Save(ProfileKind k){auto& p=advancedRuntime.profile(k);CanonicalizeProfile(p);EnforceAccuracyMethod(p);CanonicalizeProfile(p);ProfileValue(k).SetValue(Serialize(p));}
void SaveAll(){for(std::size_t i=0;i<kProfileCount;++i)Save(static_cast<ProfileKind>(i));}

void NormalizeSimple(SimpleConfig& s){
    s.accuracyWeightPct=clampPercent(s.accuracyWeightPct);
    s.beforeWeightPct=std::min(clampPercent(s.beforeWeightPct),100-s.accuracyWeightPct);
    s.afterWeightPct=std::min(clampPercent(s.afterWeightPct),100-s.accuracyWeightPct-s.beforeWeightPct);
    s.fullNoteMax=clampMaxScore(s.fullNoteMax);
    s.chainLinkMax=clampMaxScore(s.chainLinkMax);
    s.beforeFullCreditAngleDeg=std::clamp(s.beforeFullCreditAngleDeg,1.0,180.0);
    s.afterFullCreditAngleDeg=std::clamp(s.afterFullCreditAngleDeg,1.0,180.0);
    s.preciseUpperPairWeightPct=clampPercent(s.preciseUpperPairWeightPct);
}

SimpleConfig ReadSimple(){auto&c=getCutAccuracyConfig();SimpleConfig s; s.accuracyMethod=c.SimpleAccuracyMethod.GetValue()==0?AccuracyMethod::BeatSaber:AccuracyMethod::Precise;s.fullNoteMax=std::clamp(c.SimpleFullNoteMax.GetValue(),0,1000);s.chainLinkMax=std::clamp(c.SimpleChainLinkMax.GetValue(),0,1000);s.accuracyWeightPct=clampPercent(c.AccuracyWeightPct.GetValue());s.beforeWeightPct=clampPercent(c.SimpleBeforeWeightPct.GetValue());s.afterWeightPct=clampPercent(c.SimpleAfterWeightPct.GetValue());s.beforeFullCreditAngleDeg=std::clamp(c.SimpleBeforeAngle.GetValue(),1,180);s.afterFullCreditAngleDeg=std::clamp(c.SimpleAfterAngle.GetValue(),1,180);s.preciseUpperPairWeightPct=clampPercent(c.SimpleUpperPairWeightPct.GetValue());NormalizeSimple(s);return s;}
void WriteSimple(const SimpleConfig&input){auto s=input;NormalizeSimple(s);auto&c=getCutAccuracyConfig();c.SimpleAccuracyMethod.SetValue(s.accuracyMethod==AccuracyMethod::BeatSaber?0:1);c.SimpleFullNoteMax.SetValue(static_cast<int>(std::lround(s.fullNoteMax)));c.SimpleChainLinkMax.SetValue(static_cast<int>(std::lround(s.chainLinkMax)));c.AccuracyWeightPct.SetValue(clampPercent(s.accuracyWeightPct));c.SimpleBeforeWeightPct.SetValue(clampPercent(s.beforeWeightPct));c.SimpleAfterWeightPct.SetValue(clampPercent(s.afterWeightPct));c.SimpleBeforeAngle.SetValue(static_cast<int>(std::lround(s.beforeFullCreditAngleDeg)));c.SimpleAfterAngle.SetValue(static_cast<int>(std::lround(s.afterFullCreditAngleDeg)));c.SimpleUpperPairWeightPct.SetValue(clampPercent(s.preciseUpperPairWeightPct));}

void InitializeRuntime(){advancedRuntime=defaultAdvancedConfig();for(std::size_t i=0;i<kProfileCount;++i){auto k=static_cast<ProfileKind>(i);ScoringProfile parsed;auto raw=ProfileValue(k).GetValue();if(Parse(raw,parsed)){CanonicalizeProfile(parsed);EnforceAccuracyMethod(parsed);advancedRuntime.profile(k)=parsed;auto canonical=Serialize(parsed);if(raw!=canonical)ProfileValue(k).SetValue(canonical);}else{CanonicalizeProfile(advancedRuntime.profile(k));EnforceAccuracyMethod(advancedRuntime.profile(k));ProfileValue(k).SetValue(Serialize(advancedRuntime.profile(k)));}}}

struct Ui {
    bool refreshing{false};
    bool moreOptionsOpen{false};
    bool displayOptionsOpen{false};
    ProfileKind selected{ProfileKind::DirectionalNote};
    ProfileKind copyFrom{ProfileKind::DirectionalNote};

    HMUI::TextSegmentedControl* modeControl{};
    UnityEngine::GameObject *offPanel{}, *simplePanel{}, *advancedPanel{}, *displayOptionsPanel{}, *advancedMorePanel{};
    UnityEngine::GameObject *accuracyMixRow{}, *preciseUpperRow{}, *beforeAngleRow{}, *afterAngleRow{};
    UnityEngine::GameObject *badValueRow{}, *missValueRow{}, *scoreTextPanel{}, *simpleUpperRow{};
    BSML::DropdownListSetting *advancedMethod{}, *simpleMethod{}, *profile{}, *copyProfile{}, *badMode{}, *missMode{};
    BSML::ToggleSetting *moreOptions{}, *displayOptions{}, *flyingScore{}, *captionText{}, *editCaptions{};

    BSML::SliderSetting *sAcc{}, *sBefore{}, *sAfter{}, *sBeforeAngle{}, *sAfterAngle{}, *sUpper{};
    HMUI::InputFieldView *sFullMax{}, *sChainMax{};

    BSML::SliderSetting *aAccuracy{}, *aMix{}, *aFlat{}, *aPrecW{}, *aUpper{}, *aCenterW{}, *aBeforeW{}, *aBeforeAngle{}, *aAfterW{}, *aAfterAngle{}, *aBadValue{}, *aMissValue{};
    HMUI::InputFieldView *aMax{};

    HMUI::CurvedTextMeshPro *simplePresetSummary{}, *simpleSummary{}, *advancedSummary{};
};


struct TypedScoreInput {
    int value{0};
    bool rewrite{false};
};

void SetScoreFieldText(const std::shared_ptr<Ui>& u, HMUI::InputFieldView* field, int value) {
    if(!u||!field)return;
    u->refreshing=true;
    field->set_text(il2cpp_utils::newcsstr(std::to_string(std::clamp(value,0,1000))));
    u->refreshing=false;
}
void NormalizeVisibleScoreField(const std::shared_ptr<Ui>& u, HMUI::InputFieldView* field, const TypedScoreInput& parsed) {
    if(parsed.rewrite)SetScoreFieldText(u,field,parsed.value);
}

std::array<std::string_view,11> profileNames={"Directional","Dot","Arc Head","Arc Tail","Chain Head","Chain Link","Arc Head+Tail","Chain Head+Tail","Chain Head+Arc","Chain Head+Arc+Tail","Link+Arc Head"};
std::array<std::string_view,3> failureNames={"Zero","Fixed pts","Percent"};
std::array<std::string_view,3> modeNames={"Off","Simple","Advanced"};
std::array<std::string_view,3> profileAccuracyNames={"Beat Saber","Precise","Blended"};
std::array<std::string_view,2> accuracyNames={"Beat Saber","Precise"};

void SetSliderBounds(BSML::SliderSetting* setting, float minValue, float maxValue) {
    if (!setting || !setting->slider) return;
    const float lo=minValue;
    const float hi=std::max(minValue,maxValue);
    setting->slider->set_minValue(lo);
    setting->slider->set_maxValue(hi);
    const float increment=std::max(0.0001f,setting->increments);
    const int intervals=std::max(0,static_cast<int>(std::lround((hi-lo)/increment)));
    setting->slider->set_numberOfSteps(std::max(1,intervals+1));
}

void MakeIntegerSlider(BSML::SliderSetting* setting) {
    if (!setting) return;
    setting->isInt = true;
    setting->digits = 0;
}

std::string Pct(int value) { return std::to_string(clampPercent(value)); }


std::optional<TypedScoreInput> TypedScoreValue(StringW value) {
    try {
        std::string text=static_cast<std::string>(value);
        if(text.empty())return std::nullopt;
        std::size_t first=0;while(first<text.size()&&std::isspace(static_cast<unsigned char>(text[first])))++first;
        std::size_t last=text.size();while(last>first&&std::isspace(static_cast<unsigned char>(text[last-1])))--last;
        if(first==last)return std::nullopt;
        for(std::size_t i=first;i<last;++i)if(!std::isdigit(static_cast<unsigned char>(text[i])))return std::nullopt;
        const auto trimmed=text.substr(first,last-first);
        long long raw=std::stoll(trimmed);
        const int bounded=static_cast<int>(std::clamp<long long>(raw,0,1000));
        return TypedScoreInput{bounded,raw!=bounded||trimmed!=std::to_string(bounded)};
    }catch(...){return std::nullopt;}
}

// Let parent layouts allocate width and propagate each panel's preferred height.
// BSML's raw group factories leave child sizing disabled and add width fitters.
void ConfigureStack(UnityEngine::UI::VerticalLayoutGroup* layout) {
    layout->set_childControlWidth(true);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandWidth(true);
    layout->set_childForceExpandHeight(false);
    layout->set_spacing(0.8f);
    layout->set_childAlignment(UnityEngine::TextAnchor::UpperLeft);
    auto* fitter=layout->get_gameObject()->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
    if(fitter)fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
}
UnityEngine::UI::VerticalLayoutGroup* Stack(const BSML::Lite::TransformWrapper& parent) {
    auto* layout=BSML::Lite::CreateVerticalLayoutGroup(parent);
    ConfigureStack(layout);
    return layout;
}
UnityEngine::UI::HorizontalLayoutGroup* Row(const BSML::Lite::TransformWrapper& parent) {
    auto* layout=BSML::Lite::CreateHorizontalLayoutGroup(parent);
    layout->set_childControlWidth(true);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandWidth(false);
    layout->set_childForceExpandHeight(false);
    layout->set_spacing(1.1f);
    auto* element=layout->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>();
    element->set_minHeight(7.2f);
    element->set_preferredHeight(7.2f);
    return layout;
}
void ReserveHeight(UnityEngine::GameObject* object, float height) {
    auto* element=object->GetComponent<UnityEngine::UI::LayoutElement*>();
    if(!element)element=object->AddComponent<UnityEngine::UI::LayoutElement*>();
    element->set_minHeight(height);
    element->set_preferredHeight(height);
    element->set_flexibleHeight(0);
}
void SetElementSize(UnityEngine::GameObject* object, float minWidth, float preferredWidth, float height, float flexibleWidth = 0.0f) {
    if(!object)return;
    auto* element=object->GetComponent<UnityEngine::UI::LayoutElement*>();
    if(!element)element=object->AddComponent<UnityEngine::UI::LayoutElement*>();
    element->set_minWidth(minWidth);
    element->set_preferredWidth(preferredWidth);
    element->set_flexibleWidth(flexibleWidth);
    element->set_minHeight(height);
    element->set_preferredHeight(height);
    element->set_flexibleHeight(0.0f);
}
template<class T>
void SetControlSize(T* control, float minWidth, float preferredWidth, float height, float flexibleWidth = 0.0f) {
    if(control)SetElementSize(control->get_gameObject(),minWidth,preferredWidth,height,flexibleWidth);
}

void Section(UnityEngine::Transform* parent, std::string_view title, std::string_view subtitle = {}) {
    auto* heading = BSML::Lite::CreateText(parent, std::string(title), TMPro::FontStyles::Bold, 3.25f);
    if (heading) ReserveHeight(heading->get_gameObject(), 4.8f);
    if (heading && !subtitle.empty()) BSML::Lite::AddHoverHint(heading->get_gameObject(), std::string(subtitle));
}
HMUI::CurvedTextMeshPro* Subtext(const BSML::Lite::TransformWrapper& parent, std::string_view text) {
    auto* label=BSML::Lite::CreateText(parent,std::string(text),TMPro::FontStyles::Normal,2.75f);
    if(label)ReserveHeight(label->get_gameObject(),4.2f);
    return label;
}

template<class T>
void StyleActionButton(T* button, float preferredWidth = 21.0f) {
    SetControlSize(button,16.0f,preferredWidth,6.6f,1.0f);
}

void RefreshModePanels(const std::shared_ptr<Ui>& u) {
    if (!u) return;
    auto& cfg = getCutAccuracyConfig();
    const int mode = std::clamp(cfg.Mode.GetValue(), 0, 2);
    if (u->offPanel) u->offPanel->SetActive(mode == 0);
    if (u->simplePanel) u->simplePanel->SetActive(mode == 1);
    if (u->advancedPanel) u->advancedPanel->SetActive(mode == 2);

    u->refreshing = true;
    if (u->modeControl) u->modeControl->SelectCellWithNumber(mode);
    u->refreshing = false;
}

void RefreshSimple(const std::shared_ptr<Ui>& u, bool refreshMaxFields=true) {
    if (!u) return;
    auto s = ReadSimple();
    u->refreshing = true;
    if (u->simpleMethod) {
        u->simpleMethod->set_Value(reinterpret_cast<System::Object*>(il2cpp_utils::newcsstr(s.accuracyMethod == AccuracyMethod::BeatSaber ? "Beat Saber" : "Precise")));
        u->simpleMethod->UpdateState();
    }
    if (refreshMaxFields && u->sFullMax) u->sFullMax->set_text(il2cpp_utils::newcsstr(std::to_string(static_cast<int>(s.fullNoteMax))));
    if (refreshMaxFields && u->sChainMax) u->sChainMax->set_text(il2cpp_utils::newcsstr(std::to_string(static_cast<int>(s.chainLinkMax))));
    SetSliderBounds(u->sAcc, 0.0f, static_cast<float>(simpleWeightMaxPct(s, ComponentId::Precise)));
    SetSliderBounds(u->sBefore, 0.0f, static_cast<float>(simpleWeightMaxPct(s, ComponentId::Before)));
    SetSliderBounds(u->sAfter, 0.0f, static_cast<float>(simpleWeightMaxPct(s, ComponentId::After)));
    if (u->sAcc) u->sAcc->set_Value(s.accuracyWeightPct);
    if (u->sBefore) u->sBefore->set_Value(s.beforeWeightPct);
    if (u->sAfter) u->sAfter->set_Value(s.afterWeightPct);
    if (u->sBeforeAngle) u->sBeforeAngle->set_Value(s.beforeFullCreditAngleDeg);
    if (u->sAfterAngle) u->sAfterAngle->set_Value(s.afterFullCreditAngleDeg);
    if (u->sUpper) u->sUpper->set_Value(s.preciseUpperPairWeightPct);
    if (u->simpleUpperRow) u->simpleUpperRow->SetActive(s.accuracyMethod == AccuracyMethod::Precise);
    if (u->simplePresetSummary) {
        const std::string method=s.accuracyMethod==AccuracyMethod::BeatSaber ? "Beat Saber center" : "Precise mini-note";
        u->simplePresetSummary->set_text(method+"    Accuracy "+std::to_string(s.accuracyWeightPct)+"    Before "+std::to_string(s.beforeWeightPct)+"    After "+std::to_string(s.afterWeightPct));
    }
    if (u->simpleSummary) {
        const int sum = s.accuracyWeightPct + s.beforeWeightPct + s.afterWeightPct;
        u->simpleSummary->set_text("Used " + std::to_string(sum) + "% / 100%    Free " + std::to_string(std::max(0, 100 - sum)) + "%");
    }
    u->refreshing = false;
}

std::string FailureName(FailureMode m) { return std::string(failureNames[std::clamp(static_cast<int>(m), 0, 2)]); }

double FailureValueMax(const ScoringProfile& p, FailureMode mode) {
    if (mode == FailureMode::FixedPoints) return p.maxScore;
    if (mode == FailureMode::PercentOfMax) return 100.0;
    return 0.0;
}

void RefreshAdvanced(const std::shared_ptr<Ui>& u, bool refreshMaxField=true) {
    if (!u) return;
    auto& p = advancedRuntime.profile(u->selected);
    CanonicalizeProfile(p);
    EnforceAccuracyMethod(p);
    u->refreshing = true;

    if (u->profile) {
        u->profile->set_Value(reinterpret_cast<System::Object*>(il2cpp_utils::newcsstr(std::string(profileNames[profileIndex(u->selected)]))));
        u->profile->UpdateState();
    }
    if (refreshMaxField && u->aMax) u->aMax->set_text(il2cpp_utils::newcsstr(std::to_string(static_cast<int>(std::lround(p.maxScore)))));

    SetSliderBounds(u->aFlat, 0.0f, static_cast<float>(componentWeightMaxPct(p, ComponentId::Flat)));
    if (u->aFlat) u->aFlat->set_Value(p.flat.weightPct);
    if(u->advancedMethod) {
        u->advancedMethod->set_Value(reinterpret_cast<System::Object*>(il2cpp_utils::newcsstr(std::string(profileAccuracyNames[static_cast<int>(p.accuracyMethod)]))));
        u->advancedMethod->UpdateState();
    }
    SetSliderBounds(u->aAccuracy,0.0f,static_cast<float>(accuracyWeightMax(p)));
    if(u->aAccuracy)u->aAccuracy->set_Value(accuracyWeight(p));
    if(u->aMix)u->aMix->set_Value(p.preciseMixPct);
    if(u->aUpper)u->aUpper->set_Value(p.precise.upperPairWeightPct);
    if(u->accuracyMixRow)u->accuracyMixRow->SetActive(p.accuracyMethod==ProfileAccuracyMethod::Blended);
    SetSliderBounds(u->aBeforeW, 0.0f, static_cast<float>(componentWeightMaxPct(p, ComponentId::Before)));
    if (u->aBeforeW) u->aBeforeW->set_Value(p.before.weightPct);
    if (u->aBeforeAngle) u->aBeforeAngle->set_Value(p.before.fullCreditAngleDeg);
    SetSliderBounds(u->aAfterW, 0.0f, static_cast<float>(componentWeightMaxPct(p, ComponentId::After)));
    if (u->aAfterW) u->aAfterW->set_Value(p.after.weightPct);
    if (u->aAfterAngle) u->aAfterAngle->set_Value(p.after.fullCreditAngleDeg);

    if (u->badMode) { u->badMode->set_Value(reinterpret_cast<System::Object*>(il2cpp_utils::newcsstr(FailureName(p.badCut.mode)))); u->badMode->UpdateState(); }
    SetSliderBounds(u->aBadValue, 0.0f, static_cast<float>(FailureValueMax(p, p.badCut.mode)));
    p.badCut.value = std::clamp(p.badCut.value, 0.0, FailureValueMax(p, p.badCut.mode));
    if (u->aBadValue) u->aBadValue->set_Value(p.badCut.value);
    if (u->missMode) { u->missMode->set_Value(reinterpret_cast<System::Object*>(il2cpp_utils::newcsstr(FailureName(p.miss.mode)))); u->missMode->UpdateState(); }
    SetSliderBounds(u->aMissValue, 0.0f, static_cast<float>(FailureValueMax(p, p.miss.mode)));
    p.miss.value = std::clamp(p.miss.value, 0.0, FailureValueMax(p, p.miss.mode));
    if (u->aMissValue) u->aMissValue->set_Value(p.miss.value);

    // Weight 0 is the OFF state. Only controls that need a non-zero component
    // remain visible, which keeps the Advanced editor close to native Beat Saber
    // settings density without sacrificing any scoring capability.
    if (u->preciseUpperRow) u->preciseUpperRow->SetActive(p.accuracyMethod!=ProfileAccuracyMethod::BeatSaber);
    if (u->beforeAngleRow) u->beforeAngleRow->SetActive(p.before.weightPct > 0);
    if (u->afterAngleRow) u->afterAngleRow->SetActive(p.after.weightPct > 0);
    if (u->badValueRow) u->badValueRow->SetActive(p.badCut.mode != FailureMode::Zero);
    if (u->missValueRow) u->missValueRow->SetActive(p.miss.mode != FailureMode::Zero);
    if (u->advancedMorePanel) u->advancedMorePanel->SetActive(u->moreOptionsOpen);
    if (u->displayOptionsPanel) u->displayOptionsPanel->SetActive(u->displayOptionsOpen);

    if (u->advancedSummary) {
        const int sum = enabledWeightSum(p);
        std::string text = std::string(profileNames[profileIndex(u->selected)]) + "    " + std::to_string(static_cast<int>(std::lround(p.maxScore))) + " max    Used " + Pct(sum) + "%";
        if (sum < 100) text += "    Free " + Pct(100 - sum) + "%";
        u->advancedSummary->set_text(text);
    }
    u->refreshing = false;
}

void AdjustSimpleMax(const std::shared_ptr<Ui>& u, bool chain, int delta) {
    auto s = ReadSimple();
    if (chain) s.chainLinkMax = std::clamp(static_cast<int>(std::lround(s.chainLinkMax)) + delta, 0, 1000);
    else s.fullNoteMax = std::clamp(static_cast<int>(std::lround(s.fullNoteMax)) + delta, 0, 1000);
    WriteSimple(s); RefreshSimple(u);
}
void AdjustSimpleWeight(const std::shared_ptr<Ui>& u, ComponentId id, int delta) {
    auto s = ReadSimple();
    int current = id == ComponentId::Before ? s.beforeWeightPct : id == ComponentId::After ? s.afterWeightPct : s.accuracyWeightPct;
    setSimpleWeight(s, id, current + delta); WriteSimple(s); RefreshSimple(u);
}
void AdjustSimpleAngle(const std::shared_ptr<Ui>& u, bool before, int delta) {
    auto s = ReadSimple();
    if (before) s.beforeFullCreditAngleDeg = std::clamp(static_cast<int>(std::lround(s.beforeFullCreditAngleDeg)) + delta, 1, 180);
    else s.afterFullCreditAngleDeg = std::clamp(static_cast<int>(std::lround(s.afterFullCreditAngleDeg)) + delta, 1, 180);
    WriteSimple(s); RefreshSimple(u);
}
void AdjustSimpleUpper(const std::shared_ptr<Ui>& u, int delta) {
    auto s = ReadSimple(); s.preciseUpperPairWeightPct = clampPercent(s.preciseUpperPairWeightPct + delta); WriteSimple(s); RefreshSimple(u);
}

void AdjustAdvancedMax(const std::shared_ptr<Ui>& u, int delta) {
    auto& p = advancedRuntime.profile(u->selected); p.maxScore = std::clamp(static_cast<int>(std::lround(p.maxScore)) + delta, 0, 1000); normalizeFailureRule(p.badCut,p.maxScore); normalizeFailureRule(p.miss,p.maxScore); Save(u->selected); RefreshAdvanced(u);
}
void AdjustAdvancedWeight(const std::shared_ptr<Ui>& u, ComponentId id, int delta) {
    auto& p = advancedRuntime.profile(u->selected); auto* c = component(p,id); if (!c) return; setComponentWeight(p,id,c->weightPct + delta); if(id==ComponentId::Flat)p.flat.enabled=p.flat.weightPct>0; Save(u->selected); RefreshAdvanced(u);
}
void AdjustAdvancedAngle(const std::shared_ptr<Ui>& u, bool before, int delta) {
    auto& p = advancedRuntime.profile(u->selected); if(before)p.before.fullCreditAngleDeg=std::clamp(static_cast<int>(std::lround(p.before.fullCreditAngleDeg))+delta,1,180);else p.after.fullCreditAngleDeg=std::clamp(static_cast<int>(std::lround(p.after.fullCreditAngleDeg))+delta,1,180); Save(u->selected); RefreshAdvanced(u);
}
void AdjustAdvancedUpper(const std::shared_ptr<Ui>& u, int delta) {
    auto& p = advancedRuntime.profile(u->selected); p.precise.upperPairWeightPct=clampPercent(p.precise.upperPairWeightPct+delta); Save(u->selected); RefreshAdvanced(u);
}
void AdjustFailure(const std::shared_ptr<Ui>& u, bool badCut, int delta) {
    auto& p = advancedRuntime.profile(u->selected); auto& rule = badCut ? p.badCut : p.miss; const double max = FailureValueMax(p,rule.mode); rule.value=std::clamp(static_cast<double>(std::lround(rule.value) + delta), 0.0, max); Save(u->selected); RefreshAdvanced(u);
}

std::string Sanitize(const std::string&v);

void AddTextSetting(UnityEngine::GameObject* c,std::string_view label,ConfigUtils::ConfigValue<std::string>&v){Section(c->get_transform(),label);BSML::Lite::CreateStringSetting(c,std::string(label),v.GetValue(),[&v](StringW x){v.SetValue(Sanitize(static_cast<std::string>(x)));});}
void AddScoreTexts(UnityEngine::GameObject*c){auto&v=getCutAccuracyConfig();AddTextSetting(c,"0-10%",v.ScoreText0);AddTextSetting(c,"10-20%",v.ScoreText10);AddTextSetting(c,"20-30%",v.ScoreText20);AddTextSetting(c,"30-40%",v.ScoreText30);AddTextSetting(c,"40-50%",v.ScoreText40);AddTextSetting(c,"50-60%",v.ScoreText50);AddTextSetting(c,"60-70%",v.ScoreText60);AddTextSetting(c,"70-80%",v.ScoreText70);AddTextSetting(c,"80-90%",v.ScoreText80);AddTextSetting(c,"90-100%",v.ScoreText90);AddTextSetting(c,"100%",v.ScoreText100);}

void BuildSettingsMenu(HMUI::ViewController*view,bool first,bool,bool){
    if(!first||!view)return;
    auto*c=BSML::Lite::CreateScrollableSettingsContainer(view->get_transform());
    if(auto* layout=c->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>()) ConfigureStack(layout);
    // The outer scroll content follows the viewport width, while its fitter
    // computes only height. Otherwise long dropdown labels widen the whole menu.
    auto contentParent=c->get_transform()->get_parent()->get_gameObject();
    if(auto* fitter=contentParent->GetComponent<UnityEngine::UI::ContentSizeFitter*>())
        fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained);
    if(auto* layout=contentParent->GetComponent<UnityEngine::UI::VerticalLayoutGroup*>())
        layout->set_childForceExpandWidth(true);
    auto&cfg=getCutAccuracyConfig();
    auto u=std::make_shared<Ui>();

    auto* title=BSML::Lite::CreateText(c,"Cut Accuracy",TMPro::FontStyles::Bold,3.9f);
    if(title)ReserveHeight(title->get_gameObject(),5.4f);

    Section(c->get_transform(),"Mode","Off uses vanilla Beat Saber scoring. Simple and Advanced use custom scoring and keep external score submission disabled while selected.");
    auto* modeRow=Row(c);
    modeRow->set_childAlignment(UnityEngine::TextAnchor::MiddleCenter);
    u->modeControl=BSML::Lite::CreateTextSegmentedControl(
        modeRow,{0.0f,0.0f},{48.0f,6.6f},modeNames,[u,&cfg](int selected){
            if(u->refreshing)return;
            cfg.Mode.SetValue(std::clamp(selected,0,2));
            UpdateScoreSubmissionPolicy();
            RefreshModePanels(u);
        });

    Section(c->get_transform(),"Profile tools","Copy or reset Advanced note-type profiles.");
    u->profile=BSML::Lite::CreateDropdown(c,"Note type",profileNames[0],profileNames,[u](StringW v){if(u->refreshing)return;std::string s=v;for(std::size_t i=0;i<profileNames.size();++i)if(s==profileNames[i]){u->selected=static_cast<ProfileKind>(i);break;}RefreshAdvanced(u);});
    u->copyProfile=BSML::Lite::CreateDropdown(c,"Copy from",profileNames[0],profileNames,[u](StringW v){std::string s=v;for(std::size_t i=0;i<profileNames.size();++i)if(s==profileNames[i]){u->copyFrom=static_cast<ProfileKind>(i);break;}});
    BSML::Lite::AddHoverHint(u->profile->get_gameObject(),"Advanced note type being edited.");
    BSML::Lite::AddHoverHint(u->copyProfile->get_gameObject(),"Source note type for Copy and Copy All.");
    auto* profileActions=Row(c);
    profileActions->set_spacing(1.0f);
    auto* copy=BSML::Lite::CreateUIButton(profileActions,"Copy",[u](){advancedRuntime.profile(u->selected)=advancedRuntime.profile(u->copyFrom);Save(u->selected);RefreshAdvanced(u);});
    auto* copyAll=BSML::Lite::CreateUIButton(profileActions,"Copy All",[u](){auto source=advancedRuntime.profile(u->copyFrom);for(std::size_t i=0;i<kProfileCount;++i)advancedRuntime.profile(static_cast<ProfileKind>(i))=source;SaveAll();RefreshAdvanced(u);});
    auto* resetProfile=BSML::Lite::CreateUIButton(profileActions,"Reset",[u](){advancedRuntime.profile(u->selected)=defaultAdvancedConfig().profile(u->selected);Save(u->selected);RefreshAdvanced(u);});
    auto* resetAll=BSML::Lite::CreateUIButton(profileActions,"Reset All",[u](){advancedRuntime=defaultAdvancedConfig();SaveAll();RefreshAdvanced(u);});
    StyleActionButton(copy,18.0f);
    StyleActionButton(copyAll,22.0f);
    StyleActionButton(resetProfile,18.0f);
    StyleActionButton(resetAll,23.0f);
    BSML::Lite::AddHoverHint(copy->get_gameObject(),"Replace the selected note type with the profile chosen in Copy from.");
    BSML::Lite::AddHoverHint(copyAll->get_gameObject(),"Apply the profile chosen in Copy from to every Advanced note type.");
    BSML::Lite::AddHoverHint(resetProfile->get_gameObject(),"Restore the selected note type to its Beat Saber-like default.");
    BSML::Lite::AddHoverHint(resetAll->get_gameObject(),"Restore every Advanced note type to its Beat Saber-like default.");

    u->displayOptions=BSML::Lite::CreateToggle(c,"Extras",false,[u](bool v){u->displayOptionsOpen=v;if(u->displayOptionsPanel)u->displayOptionsPanel->SetActive(v);});
    auto* displayOptionsLayout=Stack(c); u->displayOptionsPanel=displayOptionsLayout->get_gameObject();
    u->flyingScore=BSML::Lite::CreateToggle(displayOptionsLayout,"Flying score",cfg.ShowFlyingScore.GetValue(),[u,&cfg](bool v){cfg.ShowFlyingScore.SetValue(v);if(u->captionText)u->captionText->get_gameObject()->SetActive(v);if(u->editCaptions)u->editCaptions->get_gameObject()->SetActive(v&&cfg.ShowScoreText.GetValue());if(u->scoreTextPanel)u->scoreTextPanel->SetActive(v&&cfg.ShowScoreText.GetValue()&&cfg.EditScoreTextLabels.GetValue());});
    if(u->flyingScore)BSML::Lite::AddHoverHint(u->flyingScore->get_gameObject(),"Show the custom per-cut flying score. Turn this off to leave Beat Saber's native flying-score display alone.");
    u->captionText=BSML::Lite::CreateToggle(displayOptionsLayout,"Caption text",cfg.ShowScoreText.GetValue(),[u,&cfg](bool v){cfg.ShowScoreText.SetValue(v);if(u->editCaptions)u->editCaptions->get_gameObject()->SetActive(cfg.ShowFlyingScore.GetValue()&&v);if(u->scoreTextPanel)u->scoreTextPanel->SetActive(cfg.ShowFlyingScore.GetValue()&&v&&cfg.EditScoreTextLabels.GetValue());});
    if(u->captionText)BSML::Lite::AddHoverHint(u->captionText->get_gameObject(),"Add a short label below the flying score using percentage bands.");
    u->editCaptions=BSML::Lite::CreateToggle(displayOptionsLayout,"Edit captions",cfg.EditScoreTextLabels.GetValue(),[u,&cfg](bool v){cfg.EditScoreTextLabels.SetValue(v);if(u->scoreTextPanel)u->scoreTextPanel->SetActive(cfg.ShowFlyingScore.GetValue()&&cfg.ShowScoreText.GetValue()&&v);});
    if(u->captionText)u->captionText->get_gameObject()->SetActive(cfg.ShowFlyingScore.GetValue());
    if(u->editCaptions)u->editCaptions->get_gameObject()->SetActive(cfg.ShowFlyingScore.GetValue()&&cfg.ShowScoreText.GetValue());
    auto* captionLayout=Stack(displayOptionsLayout); u->scoreTextPanel=captionLayout->get_gameObject();
    Section(captionLayout->get_transform(),"Caption bands","These labels are chosen by flying-score accuracy percentage, not by raw points.");
    AddScoreTexts(u->scoreTextPanel);
    u->scoreTextPanel->SetActive(cfg.ShowFlyingScore.GetValue()&&cfg.ShowScoreText.GetValue()&&cfg.EditScoreTextLabels.GetValue());
    u->displayOptionsPanel->SetActive(false);

    auto* offLayout=Stack(c); u->offPanel=offLayout->get_gameObject();
    auto* offText=BSML::Lite::CreateText(offLayout,"Vanilla Beat Saber scoring is active.",TMPro::FontStyles::Normal,3.2f);
    if(offText) BSML::Lite::AddHoverHint(offText->get_gameObject(),"Cut Accuracy does not replace note scoring while Off is selected.");

    auto* simpleLayout=Stack(c); u->simplePanel=simpleLayout->get_gameObject();
    Section(simpleLayout->get_transform(),"Simple","Choose a preset, then adjust the method and weights.");
    auto* simplePresets=Row(simpleLayout);
    auto* nativePreset=BSML::Lite::CreateUIButton(simplePresets,"Native",[u](){auto s=ReadSimple();s.accuracyMethod=AccuracyMethod::BeatSaber;s.accuracyWeightPct=13;s.beforeWeightPct=61;s.afterWeightPct=26;s.beforeFullCreditAngleDeg=100;s.afterFullCreditAngleDeg=60;WriteSimple(s);RefreshSimple(u);});
    auto* balancedPreset=BSML::Lite::CreateUIButton(simplePresets,"Balanced",[u](){auto s=ReadSimple();s.accuracyMethod=AccuracyMethod::Precise;s.accuracyWeightPct=40;s.beforeWeightPct=30;s.afterWeightPct=30;WriteSimple(s);RefreshSimple(u);});
    auto* accuracyPreset=BSML::Lite::CreateUIButton(simplePresets,"Accuracy",[u](){auto s=ReadSimple();s.accuracyMethod=AccuracyMethod::Precise;s.accuracyWeightPct=60;s.beforeWeightPct=20;s.afterWeightPct=20;WriteSimple(s);RefreshSimple(u);});
    auto* precisePreset=BSML::Lite::CreateUIButton(simplePresets,"Precision",[u](){auto s=ReadSimple();s.accuracyMethod=AccuracyMethod::Precise;s.accuracyWeightPct=100;s.beforeWeightPct=0;s.afterWeightPct=0;WriteSimple(s);RefreshSimple(u);});
    for(auto* button:{nativePreset,balancedPreset,accuracyPreset,precisePreset})SetControlSize(button,15.0f,20.0f,6.6f,1.0f);
    BSML::Lite::AddHoverHint(nativePreset->get_gameObject(),"Beat Saber-like scoring: center distance, before swing, and after swing.");
    BSML::Lite::AddHoverHint(balancedPreset->get_gameObject(),"Precise scoring with 40% accuracy, 30% before swing, 30% after swing.");
    BSML::Lite::AddHoverHint(accuracyPreset->get_gameObject(),"Precise scoring with 60% accuracy, 20% before swing, 20% after swing.");
    BSML::Lite::AddHoverHint(precisePreset->get_gameObject(),"Precise scoring with all valid-cut points coming from accuracy.");
    u->simplePresetSummary=Subtext(simpleLayout,"");
    u->simpleMethod=BSML::Lite::CreateDropdown(simpleLayout,"Accuracy method",accuracyNames[cfg.SimpleAccuracyMethod.GetValue()==0?0:1],accuracyNames,[u,&cfg](StringW v){if(u->refreshing)return;cfg.SimpleAccuracyMethod.SetValue(static_cast<std::string>(v)=="Beat Saber"?0:1);RefreshSimple(u);});
    BSML::Lite::AddHoverHint(u->simpleMethod->get_gameObject(),"Beat Saber uses center distance. Precise uses the four mini-note balance.");

    Section(simpleLayout->get_transform(),"Score");
    auto* row=Row(simpleLayout);
    BSML::Lite::CreateUIButton(row,"-",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustSimpleMax(u,false,-1);});
    u->sFullMax=BSML::Lite::CreateStringSetting(row,"Full note",std::to_string(static_cast<int>(ReadSimple().fullNoteMax)),[u](StringW v){if(u->refreshing)return;auto s=ReadSimple();auto parsed=TypedScoreValue(v);if(!parsed){SetScoreFieldText(u,u->sFullMax,static_cast<int>(std::lround(s.fullNoteMax)));return;}s.fullNoteMax=parsed->value;WriteSimple(s);NormalizeVisibleScoreField(u,u->sFullMax,*parsed);RefreshSimple(u,false);});
    BSML::Lite::CreateUIButton(row,"+",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustSimpleMax(u,false,1);});
    row=Row(simpleLayout);
    BSML::Lite::CreateUIButton(row,"-",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustSimpleMax(u,true,-1);});
    u->sChainMax=BSML::Lite::CreateStringSetting(row,"Chain link",std::to_string(static_cast<int>(ReadSimple().chainLinkMax)),[u](StringW v){if(u->refreshing)return;auto s=ReadSimple();auto parsed=TypedScoreValue(v);if(!parsed){SetScoreFieldText(u,u->sChainMax,static_cast<int>(std::lround(s.chainLinkMax)));return;}s.chainLinkMax=parsed->value;WriteSimple(s);NormalizeVisibleScoreField(u,u->sChainMax,*parsed);RefreshSimple(u,false);});
    BSML::Lite::CreateUIButton(row,"+",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustSimpleMax(u,true,1);});

    Section(simpleLayout->get_transform(),"Weights","Whole percentages. Each slider stops at the percentage left after the other weights.");
    row=Row(simpleLayout); u->sAcc=BSML::Lite::CreateSliderSetting(row,"Accuracy",1,ReadSimple().accuracyWeightPct,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();setSimpleWeight(s,ComponentId::Precise,std::lround(v));WriteSimple(s);RefreshSimple(u);});  row->set_childForceExpandWidth(true);
    row=Row(simpleLayout); u->sBefore=BSML::Lite::CreateSliderSetting(row,"Before",1,ReadSimple().beforeWeightPct,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();setSimpleWeight(s,ComponentId::Before,std::lround(v));WriteSimple(s);RefreshSimple(u);});  row->set_childForceExpandWidth(true);
    row=Row(simpleLayout); u->sAfter=BSML::Lite::CreateSliderSetting(row,"After",1,ReadSimple().afterWeightPct,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();setSimpleWeight(s,ComponentId::After,std::lround(v));WriteSimple(s);RefreshSimple(u);});  row->set_childForceExpandWidth(true);
    u->simpleSummary=BSML::Lite::CreateText(simpleLayout,"",TMPro::FontStyles::Normal,3.0f);

    Section(simpleLayout->get_transform(),"Swing detail");
    row=Row(simpleLayout); u->sBeforeAngle=BSML::Lite::CreateSliderSetting(row,"Before angle",1,ReadSimple().beforeFullCreditAngleDeg,1,180,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();s.beforeFullCreditAngleDeg=std::lround(v);WriteSimple(s);});  row->set_childForceExpandWidth(true);
    row=Row(simpleLayout); u->sAfterAngle=BSML::Lite::CreateSliderSetting(row,"After angle",1,ReadSimple().afterFullCreditAngleDeg,1,180,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();s.afterFullCreditAngleDeg=std::lround(v);WriteSimple(s);});  row->set_childForceExpandWidth(true);
    row=Row(simpleLayout); u->simpleUpperRow=row->get_gameObject(); u->sUpper=BSML::Lite::CreateSliderSetting(row,"Upper share",1,ReadSimple().preciseUpperPairWeightPct,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto s=ReadSimple();s.preciseUpperPairWeightPct=std::lround(v);WriteSimple(s);RefreshSimple(u);});  row->set_childForceExpandWidth(true);
    for(auto* s:{u->sAcc,u->sBefore,u->sAfter,u->sBeforeAngle,u->sAfterAngle,u->sUpper})MakeIntegerSlider(s);

    auto* advancedLayout=Stack(c); u->advancedPanel=advancedLayout->get_gameObject();
    Section(advancedLayout->get_transform(),"Advanced","Edit the selected note type from Advanced tools above.");
    u->advancedSummary=BSML::Lite::CreateText(advancedLayout,"",TMPro::FontStyles::Normal,3.0f);

    Section(advancedLayout->get_transform(),"Score","Maximum score is keyboard-editable from 0 to 1000. It changes the point scale; accuracy remains normalized by max.");
    row=Row(advancedLayout);
    BSML::Lite::CreateUIButton(row,"-",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustAdvancedMax(u,-1);});
    u->aMax=BSML::Lite::CreateStringSetting(row,"Max","115",[u](StringW v){if(u->refreshing)return;auto&p=advancedRuntime.profile(u->selected);auto parsed=TypedScoreValue(v);if(!parsed){SetScoreFieldText(u,u->aMax,static_cast<int>(std::lround(p.maxScore)));return;}p.maxScore=parsed->value;CanonicalizeProfile(p);EnforceAccuracyMethod(p);Save(u->selected);NormalizeVisibleScoreField(u,u->aMax,*parsed);RefreshAdvanced(u,false);});
    BSML::Lite::CreateUIButton(row,"+",UnityEngine::Vector2{0,0},UnityEngine::Vector2{7,7},[u](){AdjustAdvancedMax(u,1);});

    Section(advancedLayout->get_transform(),"Accuracy method","Beat Saber is center distance. Precise is four mini-note balance. Blended combines both.");
    u->advancedMethod=BSML::Lite::CreateDropdown(advancedLayout,"Method",profileAccuracyNames[0],profileAccuracyNames,[u](StringW value){
        if(u->refreshing)return;
        const std::string name=value;
        auto& p=advancedRuntime.profile(u->selected);
        const auto method=name=="Precise" ? ProfileAccuracyMethod::Precise : name=="Blended" ? ProfileAccuracyMethod::Blended : ProfileAccuracyMethod::BeatSaber;
        setAccuracyMethod(p,method);Save(u->selected);RefreshAdvanced(u);
    });
    row=Row(advancedLayout); row->set_childForceExpandWidth(true);
    u->aAccuracy=BSML::Lite::CreateSliderSetting(row,"Weight",1,0,0,100,0,true,{0,0},[u](float value){
        if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);
        setAccuracyWeight(p,std::lround(value));Save(u->selected);RefreshAdvanced(u);
    });
    row=Row(advancedLayout); row->set_childForceExpandWidth(true);u->accuracyMixRow=row->get_gameObject();
    u->aMix=BSML::Lite::CreateSliderSetting(row,"Blend",1,50,0,100,0,true,{0,0},[u](float value){
        if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);
        p.preciseMixPct=clampPercent(std::lround(value));setAccuracyWeight(p,accuracyWeight(p));Save(u->selected);RefreshAdvanced(u);
    });
    row=Row(advancedLayout); row->set_childForceExpandWidth(true);u->preciseUpperRow=row->get_gameObject();
    u->aUpper=BSML::Lite::CreateSliderSetting(row,"Upper",1,50,0,100,0,true,{0,0},[u](float value){
        if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);
        p.precise.upperPairWeightPct=clampPercent(std::lround(value));Save(u->selected);RefreshAdvanced(u);
    });
    BSML::Lite::AddHoverHint(u->advancedMethod->get_gameObject(),"Beat Saber uses distance from the note center. Precise measures the balance of four mini-notes. Blended combines both.");
    BSML::Lite::AddHoverHint(u->aAccuracy->get_gameObject(),"Share of maximum score awarded for the selected accuracy method. Accuracy, Before, After and Flat share 100%.");
    BSML::Lite::AddHoverHint(u->aMix->get_gameObject(),"Within accuracy: 0% is all Beat Saber, 100% is all Precise. The remaining share uses Beat Saber center distance.");
    BSML::Lite::AddHoverHint(u->aUpper->get_gameObject(),"Precise balance: 50% weights upper and lower pairs equally; the remainder goes to the lower pair.");
    Section(advancedLayout->get_transform(),"Other weights");
    row=Row(advancedLayout); row->set_childForceExpandWidth(true);
    u->aFlat=BSML::Lite::CreateSliderSetting(row,"Flat %",1,0,0,100,0,true,{0,0},[u](float value){if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);setComponentWeight(p,ComponentId::Flat,std::lround(value));Save(u->selected);RefreshAdvanced(u);});
    row=Row(advancedLayout); u->aBeforeW=BSML::Lite::CreateSliderSetting(row,"Before",1,0,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto&p=advancedRuntime.profile(u->selected);setComponentWeight(p,ComponentId::Before,static_cast<int>(std::lround(v)));Save(u->selected);RefreshAdvanced(u);});  row->set_childForceExpandWidth(true);
    row=Row(advancedLayout); u->aAfterW=BSML::Lite::CreateSliderSetting(row,"After",1,0,0,100,0,true,{0,0},[u](float v){if(u->refreshing)return;auto&p=advancedRuntime.profile(u->selected);setComponentWeight(p,ComponentId::After,static_cast<int>(std::lround(v)));Save(u->selected);RefreshAdvanced(u);});  row->set_childForceExpandWidth(true);
    for(auto* s:{u->aFlat,u->aAccuracy,u->aMix,u->aUpper,u->aBeforeW,u->aAfterW})MakeIntegerSlider(s);

    Section(advancedLayout->get_transform(),"Swing detail");
    row=Row(advancedLayout); u->beforeAngleRow=row->get_gameObject(); u->aBeforeAngle=BSML::Lite::CreateSliderSetting(row,"Before angle",1,100,1,180,0,true,{0,0},[u](float v){if(u->refreshing)return;auto&p=advancedRuntime.profile(u->selected);p.before.fullCreditAngleDeg=std::lround(v);Save(u->selected);});  row->set_childForceExpandWidth(true);
    row=Row(advancedLayout); u->afterAngleRow=row->get_gameObject(); u->aAfterAngle=BSML::Lite::CreateSliderSetting(row,"After angle",1,60,1,180,0,true,{0,0},[u](float v){if(u->refreshing)return;auto&p=advancedRuntime.profile(u->selected);p.after.fullCreditAngleDeg=std::lround(v);Save(u->selected);});  row->set_childForceExpandWidth(true);
    MakeIntegerSlider(u->aBeforeAngle); MakeIntegerSlider(u->aAfterAngle);

    u->moreOptions=BSML::Lite::CreateToggle(advancedLayout,"Failure scoring",false,[u](bool v){u->moreOptionsOpen=v;if(u->advancedMorePanel)u->advancedMorePanel->SetActive(v);});
    auto* moreLayout=Stack(advancedLayout); u->advancedMorePanel=moreLayout->get_gameObject();
    Section(moreLayout->get_transform(),"Failures","Bad-cut and miss scoring bypasses valid-cut component weights.");
    u->badMode=BSML::Lite::CreateDropdown(moreLayout,"Bad cut",failureNames[0],failureNames,[u](StringW v){if(u->refreshing)return;std::string s=v;auto&p=advancedRuntime.profile(u->selected);p.badCut.mode=s=="Fixed pts"?FailureMode::FixedPoints:s=="Percent"?FailureMode::PercentOfMax:FailureMode::Zero;normalizeFailureRule(p.badCut,p.maxScore);Save(u->selected);RefreshAdvanced(u);});
    row=Row(moreLayout); u->badValueRow=row->get_gameObject(); u->aBadValue=BSML::Lite::CreateSliderSetting(row,"Bad cut %/pts",1,0,0,1000,0,true,{0,0},[u](float v){if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);p.badCut.value=std::clamp(static_cast<double>(std::lround(v)),0.0,FailureValueMax(p,p.badCut.mode));Save(u->selected);RefreshAdvanced(u);});  row->set_childForceExpandWidth(true);
    u->missMode=BSML::Lite::CreateDropdown(moreLayout,"Miss",failureNames[0],failureNames,[u](StringW v){if(u->refreshing)return;std::string s=v;auto&p=advancedRuntime.profile(u->selected);p.miss.mode=s=="Fixed pts"?FailureMode::FixedPoints:s=="Percent"?FailureMode::PercentOfMax:FailureMode::Zero;normalizeFailureRule(p.miss,p.maxScore);Save(u->selected);RefreshAdvanced(u);});
    row=Row(moreLayout); u->missValueRow=row->get_gameObject(); u->aMissValue=BSML::Lite::CreateSliderSetting(row,"Miss %/pts",1,0,0,1000,0,true,{0,0},[u](float v){if(u->refreshing)return;auto& p=advancedRuntime.profile(u->selected);p.miss.value=std::clamp(static_cast<double>(std::lround(v)),0.0,FailureValueMax(p,p.miss.mode));Save(u->selected);RefreshAdvanced(u);});  row->set_childForceExpandWidth(true);
    MakeIntegerSlider(u->aBadValue); MakeIntegerSlider(u->aMissValue);

    BSML::Lite::AddHoverHint(u->aFlat->get_gameObject(),"Guaranteed percent of this note type's max on a valid cut, regardless of accuracy or swing.");
    if(u->aBadValue)BSML::Lite::AddHoverHint(u->aBadValue->get_gameObject(),"When mode is Percent, this is % of max. When mode is Fixed pts, this is raw points.");
    if(u->aMissValue)BSML::Lite::AddHoverHint(u->aMissValue->get_gameObject(),"When mode is Percent, this is % of max. When mode is Fixed pts, this is raw points.");
    u->advancedMorePanel->SetActive(false);

    SetControlSize(u->modeControl,48.0f,48.0f,6.6f);
    ReserveHeight(u->simplePresetSummary->get_gameObject(), 4.2f);
    ReserveHeight(u->simpleSummary->get_gameObject(), 4.8f);
    ReserveHeight(u->advancedSummary->get_gameObject(), 4.8f);
    for(auto* field:{u->sFullMax,u->sChainMax,u->aMax}) {
        auto* element=field->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>();
        if(!element)element=field->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        element->set_minWidth(35.0f);
        element->set_preferredWidth(50.0f);
        element->set_flexibleWidth(1.0f);
        ReserveHeight(field->get_gameObject(),8.0f);
    }
    RefreshSimple(u);
    RefreshAdvanced(u);
    RefreshModePanels(u);
}

int Bucket(double pct){const double c=std::isfinite(pct)?std::clamp(pct,0.0,100.0):0.0;if(c>=99.999999)return 10;const int whole=std::clamp(static_cast<int>(std::floor(c)),0,99);return whole/10;} std::string ScoreText(int b){auto&c=getCutAccuracyConfig();switch(b){case 0:return c.ScoreText0.GetValue();case 1:return c.ScoreText10.GetValue();case 2:return c.ScoreText20.GetValue();case 3:return c.ScoreText30.GetValue();case 4:return c.ScoreText40.GetValue();case 5:return c.ScoreText50.GetValue();case 6:return c.ScoreText60.GetValue();case 7:return c.ScoreText70.GetValue();case 8:return c.ScoreText80.GetValue();case 9:return c.ScoreText90.GetValue();default:return c.ScoreText100.GetValue();}} std::string Sanitize(const std::string&v){std::string o;for(char ch:v){unsigned char b=static_cast<unsigned char>(ch);if(ch=='<'||ch=='>'||ch=='\n'||ch=='\r'||b<0x20||b==0x7f)continue;o.push_back(ch);if(o.size()>=24)break;}return o;}
}

void InitConfig(const modloader::ModInfo&info){CutAccuracyConfig_t::Init(info);auto&c=getCutAccuracyConfig();c.Mode.SetValue(std::clamp(c.Mode.GetValue(),0,2));auto simple=ReadSimple();WriteSimple(simple);if(!c.SimpleWeightsInitialized.GetValue())c.SimpleWeightsInitialized.SetValue(true);InitializeRuntime();}
void RegisterSettingsMenu(){try{BSML::Register::RegisterSettingsMenu("Cut Accuracy",BuildSettingsMenu,false);BSML::Register::RegisterMainMenuViewControllerMethod("Cut Accuracy","Cut Accuracy","Build Simple or per-note Advanced scoring profiles",BuildSettingsMenu);}catch(...) {CutAccuracyLogger.warn("CutAccuracy settings registration failed");}}
ScoringMode CurrentScoringMode(){return static_cast<ScoringMode>(std::clamp(getCutAccuracyConfig().Mode.GetValue(),0,2));} AccuracyMethod CurrentSimpleAccuracyMethod(){return ReadSimple().accuracyMethod;} SimpleConfig CurrentSimpleConfig(){return ReadSimple();} const AdvancedConfig& CurrentAdvancedConfig(){return advancedRuntime;} ScoringProfile CurrentProfile(ProfileKind k){if(k==ProfileKind::Excluded)return{};auto m=CurrentScoringMode();if(m==ScoringMode::Advanced)return advancedRuntime.profile(k);return profileFromSimple(ReadSimple(),k);} bool CustomScoringActive(){return customScoringActiveForMode(CurrentScoringMode());} bool ShouldShowFlyingScore(){return getCutAccuracyConfig().ShowFlyingScore.GetValue();}
bool ShouldShowFlyingScoreText(){return getCutAccuracyConfig().ShowScoreText.GetValue();} std::string FlyingScoreTextForAccuracy(double pct){return ShouldShowFlyingScoreText()?Sanitize(ScoreText(Bucket(pct))):std::string{};}
}
