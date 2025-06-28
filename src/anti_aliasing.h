#pragma once
#include "stdafx.h"
#include "globals.h"
#include "pattern.h"
#include "util.h"
#include <safetyhook.hpp>
#include <spdlog/spdlog.h>

static void HookTemporalAASamples()
{
    if (std::uint8_t* result = PatternScan(exe_module, "8B ?? 18 ?? 8B ?? ?? 8B ?? 10 89 ?? 48 ?? ??"))
    {
        spdlog::info("Found address for TemporalAASamples: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid hook = safetyhook::create_mid(result + 0x0A, [](safetyhook::Context& ctx) {
            if (ctx.rax == 2)
            {
                spdlog::debug("Found TemporalAASamples value of {:d}", ctx.rax);
                LogAllValues("TemporalAASamples", ctx);
            }
        });
    } else
    {
        spdlog::error("Failed to find address for TemporalAASamples!");
    }
}
