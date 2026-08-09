#include "main.hpp"
#include "Quest/QountersHud.hpp"
#include "Quest/Settings.hpp"
#include "custom-types/shared/register.hpp"

MOD_EXPORT void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(CutAccuracyLogger.tag);
    CutAccuracyQuest::InitConfig(modInfo);
    CutAccuracyLogger.info("setup {} {} hook-safe-build-2026-08-08", MOD_ID, VERSION);
}

MOD_EXPORT void late_load() noexcept {
    try {
        il2cpp_functions::Init();
        custom_types::Register::AutoRegister();
        CutAccuracyQuest::RegisterSettingsMenu();
        CutAccuracyQuest::RegisterQountersHud();
        CutAccuracyQuest::InstallHooks();
        CutAccuracyLogger.info("CutAccuracy late_load completed score-sync-build-2026-08-08");
    } catch (const std::exception& e) {
        CutAccuracyLogger.error("CutAccuracy late_load failed: {}", e.what());
    } catch (...) {
        CutAccuracyLogger.error("CutAccuracy late_load failed with an unknown exception");
    }
}
