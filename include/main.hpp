#pragma once

#include "scotland2/shared/loader.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "paper2_scotland2/shared/logger.hpp"

#include <exception>

#ifndef MOD_EXPORT
#define MOD_EXPORT extern "C" __attribute__((visibility("default")))
#endif

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};
constexpr auto CutAccuracyLogger = Paper::ConstLoggerContext("CutAccuracy");

namespace CutAccuracyQuest {
void InstallHooks();
}
