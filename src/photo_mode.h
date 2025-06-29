#pragma once
#include "stdafx.h"
#include "globals.h"
#include "pattern.h"
#include "util.h"
#include <functional>
#include <safetyhook.hpp>
#include <spdlog/spdlog.h>

namespace PhotoMode
{
    static void HookPhotoModeHUDVisibility(std::function<void(bool)>&& fn)
    {
        if (std::uint8_t* result = PatternScan(exe_module, "0F 94 ?? 33 ?? 88 81 ?? ?? ?? ?? 48 ?? ?? C7"))
        {
            spdlog::info("Found address for PhotoMode HUD Visibility: 0x{:X}", reinterpret_cast<uintptr_t>(result));
            static std::function<void(bool)> callback = [fn](bool visible) { fn(visible); };
            static SafetyHookMid hook = safetyhook::create_mid(result + 0x05, [](safetyhook::Context& ctx) {
                if ((ctx.rdx == 1 || ctx.rdx == 0) && ctx.rbx == 0)
                {
                    const auto hud_visible = ctx.rdx == 1;
                    spdlog::debug("Found HUD toggle {:s}", hud_visible ? "true" : "false");
                    LogAllValues("HUD Visibility", ctx);
                    callback(hud_visible);
                }
            });
        } else
        {
            spdlog::error("Failed to find address for HUD Visibility!");
        }
    }

    static void HookSelfieModeActivate(std::function<void()>&& fn)
    {
        if (std::uint8_t* result = PatternScan(exe_module, "E9 ?? ?? ?? ?? 83 F8 03 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 41 B1 01"))
        {
            spdlog::info("Found address for SelfieModeActivate: 0x{:X}", reinterpret_cast<uintptr_t>(result));
            static std::function<void()> callback = [fn] { fn(); };
            static SafetyHookMid hook = safetyhook::create_mid(result, [](safetyhook::Context& ctx) {
                LogAllValues("SelfieModeActivate", ctx);
                callback();
            });
        } else
        {
            spdlog::error("Failed to find address for SelfieModeActivate!");
        }
    }

    static void HookPhotoModeActivate(std::function<void()>&& fn)
    {
        if (std::uint8_t* result = PatternScan(exe_module, "E8 ?? ?? ?? ?? 48 8B D8 89 68 08 ?? ??"))
        {
            spdlog::info("Found address for PhotoModeActivate: 0x{:X}", reinterpret_cast<uintptr_t>(result));
            static std::function<void()> callback = [fn] { fn(); };
            static SafetyHookMid hook = safetyhook::create_mid(result + 0x8, [](safetyhook::Context& ctx) {
                LogAllValues("PhotoModeActivate", ctx);
                callback();
            });
        } else
        {
            spdlog::error("Failed to find address for PhotoModeActivate!");
        }
    }

    static void HookPhotoModeDeactivate(std::function<void()>&& fn)
    {
        if (std::uint8_t* result = PatternScan(exe_module, "E8 ?? ?? ?? ?? 48 8B F0 88 58 08 E8 ?? ?? ?? ?? 48 83 78 ?? ?? 74 2E"))
        {
            spdlog::info("Found address for PhotoModeDeactivate: 0x{:X}", reinterpret_cast<uintptr_t>(result));
            static std::function<void()> callback = [fn] { fn(); };
            static SafetyHookMid hook = safetyhook::create_mid(result + 0x8, [](safetyhook::Context& ctx) {
                LogAllValues("PhotoModeDeactivate", ctx);
                callback();
            });
        } else
        {
            spdlog::error("Failed to find address for PhotoModeDeactivate!");
        }
    }
}
