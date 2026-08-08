#pragma once

#include "CutAccuracy/Presentation.hpp"

namespace GlobalNamespace { class ComboUIController; }

namespace CutAccuracyQuest {

struct HudTuning {
    float xOffset{0.0f};
    float yOffset{16.0f};
    float scale{1.0f};
    float width{840.0f};
    float height{450.0f};
    float fontSize{21.0f};
};

HudTuning& MutableHudTuning();
void MarkHudDirty();
void InstallHud(GlobalNamespace::ComboUIController* comboUI);
void ClearHud();

} // namespace CutAccuracyQuest
