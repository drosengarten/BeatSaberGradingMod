#include "Quest/Settings.hpp"
#include "main.hpp"

#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ViewController.hpp"
#include "TMPro/FontStyles.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "config-utils/shared/config-utils.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <string>
#include <string_view>

namespace CutAccuracyQuest {
namespace {

DECLARE_CONFIG(CutAccuracyConfig) {
    CONFIG_VALUE(AccuracyWeightPct, int, "AccuracyWeightPct", 13,
        "Note-accuracy weight from 0 to 100; swing-angle weight is the remainder");
    CONFIG_VALUE(ShowScoreText, bool, "ShowScoreText", true, "Show custom captions under flying cut scores");
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

struct SettingsUiState {
    BSML::SliderSetting* slider{nullptr};
    HMUI::CurvedTextMeshPro* summary{nullptr};
};

int NormalizedAccuracyWeight() {
    return CutAccuracy::clampAccuracyWeightPercent(getCutAccuracyConfig().AccuracyWeightPct.GetValue());
}

std::string WeightSummary(int accuracyPct) {
    const int clamped = CutAccuracy::clampAccuracyWeightPercent(accuracyPct);
    return "Swing angle " + std::to_string(100 - clamped)
        + "% / Note accuracy " + std::to_string(clamped) + "%";
}

void RefreshWeightSummary(const std::shared_ptr<SettingsUiState>& state) {
    if (state && state->summary) {
        state->summary->set_text(WeightSummary(NormalizedAccuracyWeight()));
    }
}

void SetAccuracyWeight(int value, const std::shared_ptr<SettingsUiState>& state, bool updateSlider) {
    const int clamped = CutAccuracy::clampAccuracyWeightPercent(value);
    getCutAccuracyConfig().AccuracyWeightPct.SetValue(clamped);
    if (updateSlider && state && state->slider) {
        state->slider->set_Value(static_cast<float>(clamped));
    }
    RefreshWeightSummary(state);
}

void NormalizeSavedAccuracyWeight() {
    const int saved = getCutAccuracyConfig().AccuracyWeightPct.GetValue();
    const int normalized = CutAccuracy::clampAccuracyWeightPercent(saved);
    if (saved != normalized) getCutAccuracyConfig().AccuracyWeightPct.SetValue(normalized);
}

void AddTextSetting(
    UnityEngine::GameObject* container,
    std::string_view label,
    ConfigUtils::ConfigValue<std::string>& configValue
) {
    BSML::Lite::CreateStringSetting(
        container,
        std::string(label),
        configValue.GetValue(),
        [&configValue](StringW value) {
            configValue.SetValue(static_cast<std::string>(value));
        }
    );
}

void AddScoreTextSettings(UnityEngine::GameObject* container) {
    auto& config = getCutAccuracyConfig();
    AddTextSetting(container, "0-9", config.ScoreText0);
    AddTextSetting(container, "10-19", config.ScoreText10);
    AddTextSetting(container, "20-29", config.ScoreText20);
    AddTextSetting(container, "30-39", config.ScoreText30);
    AddTextSetting(container, "40-49", config.ScoreText40);
    AddTextSetting(container, "50-59", config.ScoreText50);
    AddTextSetting(container, "60-69", config.ScoreText60);
    AddTextSetting(container, "70-79", config.ScoreText70);
    AddTextSetting(container, "80-89", config.ScoreText80);
    AddTextSetting(container, "90-99", config.ScoreText90);
    AddTextSetting(container, "100", config.ScoreText100);
}

int FlyingScoreTextBucket(double accuracyPct) {
    const int score = CutAccuracy::roundedClampedToInt(accuracyPct, 0, 100);
    if (score >= 100) return 10;
    return score < 10 ? 0 : score / 10;
}

std::string ConfiguredScoreText(int bucket) {
    auto& config = getCutAccuracyConfig();
    switch (bucket) {
        case 0: return config.ScoreText0.GetValue();
        case 1: return config.ScoreText10.GetValue();
        case 2: return config.ScoreText20.GetValue();
        case 3: return config.ScoreText30.GetValue();
        case 4: return config.ScoreText40.GetValue();
        case 5: return config.ScoreText50.GetValue();
        case 6: return config.ScoreText60.GetValue();
        case 7: return config.ScoreText70.GetValue();
        case 8: return config.ScoreText80.GetValue();
        case 9: return config.ScoreText90.GetValue();
        case 10:
        default:
            return config.ScoreText100.GetValue();
    }
}

std::string SanitizeScoreText(const std::string& value) {
    std::string out;
    out.reserve(std::min<std::size_t>(value.size(), 24));
    for (char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (ch == '<' || ch == '>' || ch == '\n' || ch == '\r') continue;
        if (byte < 0x20 || byte == 0x7f) continue;
        out.push_back(ch);
        if (out.size() >= 24) break;
    }
    return out;
}

void BuildSettingsMenu(HMUI::ViewController* view, bool firstActivation, bool, bool) {
    if (!firstActivation || !view) return;

    auto* container = BSML::Lite::CreateScrollableSettingsContainer(view->get_transform());
    auto& config = getCutAccuracyConfig();

    auto uiState = std::make_shared<SettingsUiState>();

    auto* presetButtons = BSML::Lite::CreateHorizontalLayoutGroup(container);
    presetButtons->spacing = 1.5f;
    BSML::Lite::CreateUIButton(presetButtons, "Classic Feel", [uiState]() { SetAccuracyWeight(0, uiState, true); });
    BSML::Lite::CreateUIButton(presetButtons, "Standard Beat Saber", [uiState]() { SetAccuracyWeight(13, uiState, true); });
    BSML::Lite::CreateUIButton(presetButtons, "Precision Mode", [uiState]() { SetAccuracyWeight(100, uiState, true); });

    uiState->slider = BSML::Lite::CreateSliderSetting(
        container,
        "Scoring Style",
        1.0f,
        static_cast<float>(NormalizedAccuracyWeight()),
        0.0f,
        100.0f,
        0.0f,
        false,
        {0.0f, 0.0f},
        [uiState](float value) {
            SetAccuracyWeight(static_cast<int>(std::lround(value)), uiState, false);
        }
    );
    if (uiState->slider) {
        uiState->slider->isInt = true;
        uiState->slider->digits = 0;
    }

    // Put the live weight summary in its own layout row. A bare CreateText child
    // in the scroll container can report too little preferred height on Quest,
    // causing the next setting to overlap it.
    auto* summaryRow = BSML::Lite::CreateHorizontalLayoutGroup(container);
    summaryRow->spacing = 0.0f;
    uiState->summary = BSML::Lite::CreateText(
        summaryRow,
        WeightSummary(NormalizedAccuracyWeight()),
        TMPro::FontStyles::Normal,
        3.5f
    );
    if (uiState->summary) {
        uiState->summary->set_enableWordWrapping(false);
        auto rect = uiState->summary->get_rectTransform();
        rect->set_sizeDelta({0.0f, 7.0f});
    }

    BSML::Lite::CreateToggle(
        container,
        "Show custom below-note text",
        config.ShowScoreText.GetValue(),
        [&config](bool value) { config.ShowScoreText.SetValue(value); }
    );
    AddScoreTextSettings(container);
}

} // namespace

void InitConfig(const modloader::ModInfo& info) {
    CutAccuracyConfig_t::Init(info);
    NormalizeSavedAccuracyWeight();
}

void RegisterSettingsMenu() {
    try {
        const bool settingsRegistered = BSML::Register::RegisterSettingsMenu("Cut Accuracy", BuildSettingsMenu, false);
        BSML::Register::RegisterMainMenuViewControllerMethod(
            "Cut Accuracy",
            "Cut Accuracy",
            "Blend swing angle and note accuracy scoring",
            BuildSettingsMenu
        );
        CutAccuracyLogger.info(
            "CutAccuracy settings menu registration {}; main menu button registered",
            settingsRegistered ? "succeeded" : "failed"
        );
    } catch (const std::exception& e) {
        CutAccuracyLogger.warn("CutAccuracy settings registration failed: {}", e.what());
    } catch (...) {
        CutAccuracyLogger.warn("CutAccuracy settings registration failed with an unknown exception");
    }
}

CutAccuracy::ScoreWeights CurrentScoreWeights() {
    return {};
}

int CurrentAccuracyWeightPercent() {
    return NormalizedAccuracyWeight();
}

bool ShouldShowFlyingScoreText() {
    return getCutAccuracyConfig().ShowScoreText.GetValue();
}

std::string FlyingScoreTextForAccuracy(double accuracyPct) {
    if (!ShouldShowFlyingScoreText()) return {};
    return SanitizeScoreText(ConfiguredScoreText(FlyingScoreTextBucket(accuracyPct)));
}

} // namespace CutAccuracyQuest
