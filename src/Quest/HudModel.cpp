#include "main.hpp"
#include "Quest/HudModel.hpp"
#include "Quest/QuestState.hpp"

#include "GlobalNamespace/ComboUIController.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "bsml/shared/BSML-Lite.hpp"

#include <exception>

namespace CutAccuracyQuest {

namespace {
TMPro::TextMeshProUGUI* panelText = nullptr;
UnityEngine::RectTransform* comboRect = nullptr;
HudTuning tuning{};

UnityEngine::Vector2 anchoredPositionAboveCombo(UnityEngine::RectTransform* comboRect) {
    const auto comboPos = comboRect->get_anchoredPosition();
    const auto comboSize = comboRect->get_sizeDelta();
    const auto comboPivot = comboRect->get_pivot();

    // Attach to the actual top edge of the live Combo text. This survives HUD
    // distance changes and most other HUD mods because the panel remains in the
    // same parent/anchor space as the Combo element.
    const float comboTop = comboPos.y + comboSize.y * (1.0f - comboPivot.y);
    return {
        comboPos.x + tuning.xOffset,
        comboTop + tuning.yOffset + tuning.height * 0.5f * tuning.scale
    };
}

} // namespace

HudTuning& MutableHudTuning() {
    return tuning;
}

void InstallHud(GlobalNamespace::ComboUIController* comboUI) {
    if (!comboUI || !comboUI->_comboText) return;

    ClearHud();

    auto comboTransformHandle = comboUI->_comboText->get_rectTransform();
    auto* comboTransform = comboTransformHandle.ptr();
    comboRect = comboTransform;
    auto parentHandle = comboTransform->get_parent();
    auto* parent = parentHandle.ptr();
    if (!parent) return;

    const auto presentation = CutAccuracy::buildHudPresentation(sessionStats);
    panelText = BSML::Lite::CreateText(
        parent,
        presentation.heading + "\n" + presentation.table);
    panelText->set_fontSize(tuning.fontSize);
    panelText->set_richText(true);
    panelText->set_color({1.0f, 1.0f, 1.0f, 0.96f});
    panelText->set_enableWordWrapping(false);
    panelText->set_alignment(TMPro::TextAlignmentOptions::Center);

    auto rectHandle = panelText->get_rectTransform();
    auto* rect = rectHandle.ptr();
    rect->set_anchorMin(comboTransform->get_anchorMin());
    rect->set_anchorMax(comboTransform->get_anchorMax());
    rect->set_pivot({0.5f, 0.5f});
    rect->set_sizeDelta({tuning.width, tuning.height});
    rect->set_anchoredPosition(anchoredPositionAboveCombo(comboTransform));
    rect->set_localScale({tuning.scale, tuning.scale, tuning.scale});
}

void MarkHudDirty() {
    if (!panelText) return;
    try {
        const auto presentation = CutAccuracy::buildHudPresentation(sessionStats);
        panelText->set_text(presentation.heading + "\n" + presentation.table);
        if (comboRect) {
            auto rectHandle = panelText->get_rectTransform();
            auto* rect = rectHandle.ptr();
            rect->set_anchorMin(comboRect->get_anchorMin());
            rect->set_anchorMax(comboRect->get_anchorMax());
            rect->set_sizeDelta({tuning.width, tuning.height});
            rect->set_anchoredPosition(anchoredPositionAboveCombo(comboRect));
            rect->set_localScale({tuning.scale, tuning.scale, tuning.scale});
        }
    } catch (const std::exception& e) {
        CutAccuracyLogger.warn("CutAccuracy HUD update failed: {}", e.what());
        ClearHud();
    } catch (...) {
        CutAccuracyLogger.warn("CutAccuracy HUD update failed with an unknown exception");
        ClearHud();
    }
}

void ClearHud() {
    // Scene teardown can invalidate Unity objects before our cached pointers
    // observe that destruction. Forget the handles and let Unity own lifetime.
    panelText = nullptr;
    comboRect = nullptr;
}

} // namespace CutAccuracyQuest
