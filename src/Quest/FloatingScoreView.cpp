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
#include <cstddef>
#include <string>
#include <unordered_map>

namespace CutAccuracyQuest {
namespace {

struct FloatingState {
    GlobalNamespace::FlyingScoreEffect* effect{nullptr};
    std::string customScore{};
    std::string customText{};
    double accuracyPct{0.0};
};

std::unordered_map<GlobalNamespace::IReadonlyCutScoreBuffer*, FloatingState> states;

void Apply(GlobalNamespace::IReadonlyCutScoreBuffer* buffer) {
    const auto it = states.find(buffer);
    if (it == states.end()) return;
    auto& state = it->second;
    if (!state.effect || state.customScore.empty() || !state.effect->_text) return;
    const auto* hex = CutAccuracy::accuracyBandHex(state.accuracyPct);
    std::string text = state.customScore;
    if (ShouldShowFlyingScoreText() && !state.customText.empty()) {
        text += "\n<size=60%>";
        text += state.customText;
        text += "</size>";
    }
    state.effect->_text->set_richText(true);
    state.effect->_text->set_alignment(TMPro::TextAlignmentOptions::Center);
    state.effect->_text->set_text("<color=" + std::string(hex) + ">" + text + "</color>");
    const auto rgb = CutAccuracy::accuracyBandRgb(state.accuracyPct);
    state.effect->_text->set_color({rgb.r, rgb.g, rgb.b, 1.0f});
}

} // namespace

void RegisterFlyingScore(
    GlobalNamespace::IReadonlyCutScoreBuffer* buffer,
    GlobalNamespace::FlyingScoreEffect* effect) {
    if (!buffer || !effect) return;

    for (auto it = states.begin(); it != states.end();) {
        if (it->second.effect == effect) it = states.erase(it);
        else ++it;
    }

    states[buffer].effect = effect;
    Apply(buffer);
}

void PresentCustomFlyingScore(
    GlobalNamespace::CutScoreBuffer* buffer,
    const CutAccuracy::NoteComponents& components) {
    if (!buffer) return;
    auto* readOnly = buffer->i___GlobalNamespace__IReadonlyCutScoreBuffer();
    states[readOnly].customScore = CutAccuracy::formatPerNoteScore(components);
    states[readOnly].accuracyPct = std::clamp(components.total(), 0.0, 100.0);
    states[readOnly].customText = FlyingScoreTextForAccuracy(states[readOnly].accuracyPct);
    Apply(readOnly);
}

void PresentFixedFlyingScore(
    GlobalNamespace::CutScoreBuffer* buffer,
    double score,
    double maxScore) {
    if (!buffer) return;
    auto* readOnly = buffer->i___GlobalNamespace__IReadonlyCutScoreBuffer();
    states[readOnly].customScore = CutAccuracy::formatFixedScore(score, maxScore);
    states[readOnly].accuracyPct = maxScore > 0.0 ? std::clamp(score / maxScore * 100.0, 0.0, 100.0) : 0.0;
    states[readOnly].customText = FlyingScoreTextForAccuracy(states[readOnly].accuracyPct);
    Apply(readOnly);
}

void ReapplyCustomFlyingScore(GlobalNamespace::IReadonlyCutScoreBuffer* buffer) {
    Apply(buffer);
}

void ReapplyCustomFlyingScore(GlobalNamespace::FlyingScoreEffect* effect) {
    if (!effect || !effect->_cutScoreBuffer) return;
    Apply(effect->_cutScoreBuffer);
}

void ClearFlyingScores() {
    states.clear();
}

} // namespace CutAccuracyQuest
