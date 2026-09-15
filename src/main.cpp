#include "main.hpp"
#include "Quest/Settings.hpp"
#include "custom-types/shared/register.hpp"
MOD_EXPORT void setup(CModInfo*info) noexcept{*info=modInfo.to_c();Paper::Logger::RegisterFileContextId(CutAccuracyLogger.tag);CutAccuracyQuest::InitConfig(modInfo);CutAccuracyLogger.info("setup {} {} configurable-profile-build",MOD_ID,VERSION);}
MOD_EXPORT void late_load() noexcept{try{il2cpp_functions::Init();custom_types::Register::AutoRegister();CutAccuracyQuest::RegisterSettingsMenu();CutAccuracyQuest::InstallHooks();CutAccuracyLogger.info("CutAccuracy late_load completed configurable-profile-build");}catch(const std::exception&e){CutAccuracyLogger.error("CutAccuracy late_load failed: {}",e.what());}catch(...){CutAccuracyLogger.error("CutAccuracy late_load failed with unknown exception");}}
