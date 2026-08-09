#include "Quest/Settings.hpp"
#include "main.hpp"

#include "HMUI/ViewController.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "config-utils/shared/config-utils.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <span>
#include <string>
#include <string_view>

namespace CutAccuracyQuest {
namespace {

DECLARE_CONFIG(CutAccuracyConfig) {
    CONFIG_VALUE(Difficulty, int, "Difficulty", 1, "Scoring profile: Easy, Normal, Hard, Expert, or Expert+");
    CONFIG_VALUE(ShowScoreText, bool, "ShowScoreText", true, "Show custom captions under flying cut scores");
    CONFIG_VALUE(ScoreText0A, std::string, "ScoreText0A", "Dire");
    CONFIG_VALUE(ScoreText0B, std::string, "ScoreText0B", "Ruined");
    CONFIG_VALUE(ScoreText10A, std::string, "ScoreText10A", "Grim");
    CONFIG_VALUE(ScoreText10B, std::string, "ScoreText10B", "Mangled");
    CONFIG_VALUE(ScoreText20A, std::string, "ScoreText20A", "Rough");
    CONFIG_VALUE(ScoreText20B, std::string, "ScoreText20B", "Botched");
    CONFIG_VALUE(ScoreText30A, std::string, "ScoreText30A", "Weak");
    CONFIG_VALUE(ScoreText30B, std::string, "ScoreText30B", "Shaky");
    CONFIG_VALUE(ScoreText40A, std::string, "ScoreText40A", "Messy");
    CONFIG_VALUE(ScoreText40B, std::string, "ScoreText40B", "Wobbly");
    CONFIG_VALUE(ScoreText50A, std::string, "ScoreText50A", "Scrappy");
    CONFIG_VALUE(ScoreText50B, std::string, "ScoreText50B", "Uneven");
    CONFIG_VALUE(ScoreText60A, std::string, "ScoreText60A", "Decent");
    CONFIG_VALUE(ScoreText60B, std::string, "ScoreText60B", "Passable");
    CONFIG_VALUE(ScoreText70A, std::string, "ScoreText70A", "Solid");
    CONFIG_VALUE(ScoreText70B, std::string, "ScoreText70B", "Respectable");
    CONFIG_VALUE(ScoreText80A, std::string, "ScoreText80A", "Sharp");
    CONFIG_VALUE(ScoreText80B, std::string, "ScoreText80B", "Clean");
    CONFIG_VALUE(ScoreText90A, std::string, "ScoreText90A", "Excellent");
    CONFIG_VALUE(ScoreText90B, std::string, "ScoreText90B", "Cracked");
    CONFIG_VALUE(ScoreText100A, std::string, "ScoreText100A", "Perfect");
    CONFIG_VALUE(ScoreText100B, std::string, "ScoreText100B", "Flawless");
};

constexpr int kNormalDifficultyIndex = 1;
static std::array<std::string_view, 5> kDifficultyNames{"Easy", "Normal", "Hard", "Expert", "Expert+"};

int NormalizedDifficultyIndex() {
    return std::clamp(getCutAccuracyConfig().Difficulty.GetValue(), 0, 4);
}

void NormalizeSavedDifficulty() {
    const int saved = getCutAccuracyConfig().Difficulty.GetValue();
    const int normalized = std::clamp(saved, 0, 4);
    if (saved != normalized) {
        getCutAccuracyConfig().Difficulty.SetValue(kNormalDifficultyIndex);
    }
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
    AddTextSetting(container, "0-9 A", config.ScoreText0A);
    AddTextSetting(container, "0-9 B", config.ScoreText0B);
    AddTextSetting(container, "10-19 A", config.ScoreText10A);
    AddTextSetting(container, "10-19 B", config.ScoreText10B);
    AddTextSetting(container, "20-29 A", config.ScoreText20A);
    AddTextSetting(container, "20-29 B", config.ScoreText20B);
    AddTextSetting(container, "30-39 A", config.ScoreText30A);
    AddTextSetting(container, "30-39 B", config.ScoreText30B);
    AddTextSetting(container, "40-49 A", config.ScoreText40A);
    AddTextSetting(container, "40-49 B", config.ScoreText40B);
    AddTextSetting(container, "50-59 A", config.ScoreText50A);
    AddTextSetting(container, "50-59 B", config.ScoreText50B);
    AddTextSetting(container, "60-69 A", config.ScoreText60A);
    AddTextSetting(container, "60-69 B", config.ScoreText60B);
    AddTextSetting(container, "70-79 A", config.ScoreText70A);
    AddTextSetting(container, "70-79 B", config.ScoreText70B);
    AddTextSetting(container, "80-89 A", config.ScoreText80A);
    AddTextSetting(container, "80-89 B", config.ScoreText80B);
    AddTextSetting(container, "90-99 A", config.ScoreText90A);
    AddTextSetting(container, "90-99 B", config.ScoreText90B);
    AddTextSetting(container, "100 A", config.ScoreText100A);
    AddTextSetting(container, "100 B", config.ScoreText100B);
}

int FlyingScoreTextBucket(double accuracyPct) {
    const int score = CutAccuracy::roundedClampedToInt(accuracyPct, 0, 100);
    if (score >= 100) return 10;
    return score < 10 ? 0 : score / 10;
}

std::string ConfiguredScoreText(int bucket, std::size_t variantSeed) {
    auto& config = getCutAccuracyConfig();
    const bool b = (variantSeed % 2) != 0;
    switch (bucket) {
        case 0: return b ? config.ScoreText0B.GetValue() : config.ScoreText0A.GetValue();
        case 1: return b ? config.ScoreText10B.GetValue() : config.ScoreText10A.GetValue();
        case 2: return b ? config.ScoreText20B.GetValue() : config.ScoreText20A.GetValue();
        case 3: return b ? config.ScoreText30B.GetValue() : config.ScoreText30A.GetValue();
        case 4: return b ? config.ScoreText40B.GetValue() : config.ScoreText40A.GetValue();
        case 5: return b ? config.ScoreText50B.GetValue() : config.ScoreText50A.GetValue();
        case 6: return b ? config.ScoreText60B.GetValue() : config.ScoreText60A.GetValue();
        case 7: return b ? config.ScoreText70B.GetValue() : config.ScoreText70A.GetValue();
        case 8: return b ? config.ScoreText80B.GetValue() : config.ScoreText80A.GetValue();
        case 9: return b ? config.ScoreText90B.GetValue() : config.ScoreText90A.GetValue();
        case 10:
        default:
            return b ? config.ScoreText100B.GetValue() : config.ScoreText100A.GetValue();
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
    AddConfigValueDropdownEnum(container, config.Difficulty, std::span<std::string_view>(kDifficultyNames));
    AddConfigValueToggle(container, config.ShowScoreText);
    AddScoreTextSettings(container);
}

} // namespace

void InitConfig(const modloader::ModInfo& info) {
    CutAccuracyConfig_t::Init(info);
    NormalizeSavedDifficulty();
}

void RegisterSettingsMenu() {
    try {
        const bool settingsRegistered = BSML::Register::RegisterSettingsMenu("Cut Accuracy", BuildSettingsMenu, false);
        BSML::Register::RegisterMainMenuViewControllerMethod(
            "Cut Accuracy",
            "Cut Accuracy",
            "Cut Accuracy difficulty settings",
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

CutAccuracy::DifficultyProfile CurrentDifficultyProfile() {
    return CutAccuracy::difficultyProfileFromIndex(NormalizedDifficultyIndex());
}

CutAccuracy::ScoreWeights CurrentScoreWeights() {
    return CutAccuracy::scoreWeightsForDifficulty(CurrentDifficultyProfile());
}

const char* CurrentDifficultyName() {
    return CutAccuracy::difficultyProfileName(CurrentDifficultyProfile());
}

bool ShouldShowFlyingScoreText() {
    return getCutAccuracyConfig().ShowScoreText.GetValue();
}

std::string FlyingScoreTextForAccuracy(double accuracyPct, std::size_t variantSeed) {
    if (!ShouldShowFlyingScoreText()) return {};
    return SanitizeScoreText(ConfiguredScoreText(FlyingScoreTextBucket(accuracyPct), variantSeed));
}

} // namespace CutAccuracyQuest
