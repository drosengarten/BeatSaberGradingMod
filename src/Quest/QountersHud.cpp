#include "Quest/QountersHud.hpp"

#include "main.hpp"
#include "Quest/QuestState.hpp"

#include "CutAccuracy/Presentation.hpp"

#include "metacore/shared/events.hpp"
#include "qounters++/shared/api.hpp"
#include "qounters++/shared/events.hpp"
#include "qounters++/shared/options.hpp"
#include "qounters++/shared/sources.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector2.hpp"

#include <exception>
#include <string>
#include <utility>

namespace CutAccuracyQuest {
namespace {

constexpr const char* kHudSourceName = "CutAccuracy HUD";
constexpr const char* kTemplateSection = "CutAccuracy";
constexpr const char* kTemplateTitle = "CutAccuracy HUD";

std::string CurrentHudText(UnparsedJSON) {
    const auto presentation = CutAccuracy::buildHudPresentation(sessionStats);
    return presentation.heading + "\n" + presentation.table;
}

void SourceOptionsUi(UnityEngine::GameObject*, UnparsedJSON) {}

void AddCutAccuracyGroup() {
    Qounters::Options::Text textOptions;
    textOptions.Align = static_cast<int>(Qounters::Options::Text::Aligns::Center);
    textOptions.Size = 12.0f;
    textOptions.TextSource = kHudSourceName;
    textOptions.SourceOptions = UnparsedJSON{};

    Qounters::Options::Component text;
    text.Type = static_cast<int>(Qounters::Options::Component::Types::Text);
    text.Options = textOptions;
    text.Position = UnityEngine::Vector2(0.0f, 0.0f);
    text.Scale = UnityEngine::Vector2(1.0f, 1.0f);

    Qounters::Options::Group group;
    group.Anchor = static_cast<int>(Qounters::Options::Group::Anchors::Top);
    group.Position = UnityEngine::Vector2(0.0f, -28.0f);
    group.Components.emplace_back(std::move(text));

    Qounters::API::AddGroup(group);
}

void AddTemplate(UnityEngine::GameObject* parent) {
    auto buttons = BSML::Lite::CreateHorizontalLayoutGroup(parent);
    buttons->spacing = 3.0f;
    BSML::Lite::CreateUIButton(buttons, "Cancel", Qounters::API::CloseTemplateModal);
    BSML::Lite::CreateUIButton(buttons, "Create", "ActionButton", []() {
        AddCutAccuracyGroup();
        Qounters::API::CloseTemplateModal();
    });
}

} // namespace

void RegisterQountersHud() {
    try {
        Qounters::Sources::RegisterText(kHudSourceName, CurrentHudText, SourceOptionsUi);
        Qounters::Events::RegisterToEvent(
            Qounters::Types::Sources::Text,
            kHudSourceName,
            static_cast<int>(MetaCore::Events::Update));
        Qounters::API::RegisterTemplate(kTemplateSection, kTemplateTitle, AddTemplate);
        CutAccuracyLogger.info("Registered CutAccuracy HUD source and template with Qounters++");
    } catch (const std::exception& e) {
        CutAccuracyLogger.warn("CutAccuracy Qounters++ HUD registration failed: {}", e.what());
    } catch (...) {
        CutAccuracyLogger.warn("CutAccuracy Qounters++ HUD registration failed with an unknown exception");
    }
}

bool ShouldUseQountersHud() {
    try {
        return Qounters::API::IsInstalled() && Qounters::API::IsEnabled();
    } catch (const std::exception& e) {
        CutAccuracyLogger.warn("CutAccuracy Qounters++ status check failed: {}", e.what());
    } catch (...) {
        CutAccuracyLogger.warn("CutAccuracy Qounters++ status check failed with an unknown exception");
    }
    return false;
}

} // namespace CutAccuracyQuest
