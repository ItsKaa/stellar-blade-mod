#include "stdafx.h"
#include "config.h"
#include "dof.h"
#include "globals.h"
#include "logging.h"
#include "photo_mode.h"
#include "screen_percentage.h"
#include <spdlog/spdlog.h>

using namespace std::literals::chrono_literals;

static void ActivatePreset(const ConfigPreset& preset, bool queue_screen_percentage = true)
{
    current_preset_name = preset.name;
    if (enable_edit_dof)
    {
        DOF::WriteDOFRecombine(preset.dof_recombine == -1 ? DOF::initial_dof_recombine : preset.dof_recombine);
        DOF::WriteDOFMaxRadius(
            std::isnan(preset.dof_max_background_radius) ? DOF::initial_dof_kernel_bg : preset.dof_max_background_radius,
            std::isnan(preset.dof_max_foreground_radius) ? DOF::initial_dof_kernel_fg : preset.dof_max_foreground_radius
        );
    }

    if (enable_edit_screen_percentage)
    {
        if (queue_screen_percentage)
        {
            std::unique_lock lock(queue_mutex);
            queue_screen_percentage_updates.push(preset.screen_percentage);
        }
        else
        {
            const auto delay = preset.type == PresetType::PhotoMode || preset.screen_percentage <= 40 ? 0 : screen_percentage_update_delay;
            WriteScreenPercentageFixLag(screen_percentage_address, preset.screen_percentage, delay);
        }
    }
}

static void HandlePhotoModeUIVisibility(bool visible)
{
    if (!enable_hud_detection)
    {
        return;
    }

    if (visible)
    {
        spdlog::info("HUD visible, activating default preset.");
        ActivatePreset(config_preset_default);
    }
    else
    {
        spdlog::info("HUD hidden, activating photos preset");
        ActivatePreset(config_preset_photos);
    }
}

static void HandlePhotoModeActivate()
{
    is_photo_mode_enabled = false;
}

static void HandlePhotoModeDeactivate()
{
    is_photo_mode_enabled = false;
    if (enable_reset_to_default)
    {
        spdlog::info("Photo mode deactivated, loading default preset.");
        ActivatePreset(config_preset_default);
    }
}

DWORD WINAPI UpdatesBackgroundThread(void*)
{
    bool shift = false, ctrl = false, alt = false;
    using namespace std::literals::chrono_literals;
    while (true)
    {
        // Screen percentage updates
        int value = 0;
        {
            std::unique_lock lock(queue_mutex);
            if (!queue_screen_percentage_updates.empty())
            {
                value = queue_screen_percentage_updates.front();
                queue_screen_percentage_updates.pop();
            }
        }
        if (enable_edit_screen_percentage && value > 0)
        {
            WriteScreenPercentageFixLag(screen_percentage_address, value, value <= config_preset_default.screen_percentage ? screen_percentage_update_delay : 0);
        }

        // Key inputs
        shift = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? true : false;
        ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
        alt = GetAsyncKeyState(VK_MENU) & 0x8000 ? true : false;

        if (!shift && !ctrl && !alt)
        {
            for (const auto& preset : config_presets)
            {
                if (::GetAsyncKeyState(preset.hotkey) & 0x8000)
                {
                    spdlog::info("Preset {} key pressed, activating", preset.name);
                    ActivatePreset(preset, false);
                    std::this_thread::sleep_for(200ms);
                }
            }
        }

        // Reload config file:
        if (ReloadConfigIfNeeded() && reload_reactivates_preset)
        {
            auto preset = GetConfigPresetByName(current_preset_name);
            if (preset.type == PresetType::Unknown)
            {
                spdlog::info("Preset with name {} could not be found, resetting to the default preset.", preset.name);
                preset = config_preset_default;
            }
            spdlog::info("Config reloaded, activating preset {:s}", preset.name);
            ActivatePreset(preset, false);
        }

        std::this_thread::sleep_for(100ms);
    }

    return 1;
}

DWORD WINAPI Main(void*)
{
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(exe_module, buffer, MAX_PATH);
    module_path = std::filesystem::path(buffer);

    InitLogging();
    LogStartupMessage();
    InitConfig();
    spdlog::info("Module Path: {:s}", module_path.string());
    spdlog::info("Module Address: 0x{:X}", reinterpret_cast<uintptr_t>(exe_module));

    PhotoMode::HookPhotoModeHUDVisibility(&HandlePhotoModeUIVisibility);
    PhotoMode::HookSelfieModeActivate(&HandlePhotoModeActivate);
    PhotoMode::HookPhotoModeActivate(&HandlePhotoModeActivate);
    PhotoMode::HookPhotoModeDeactivate(&HandlePhotoModeDeactivate);

    // Wait a bit for the heap-space scans.
    std::this_thread::sleep_for(5s);

    if (screen_percentage_address = FindScreenPercentageAddress();
        screen_percentage_address == nullptr)
    {
        spdlog::error("Unable to find the screen percentage address. Disabled the screen percentage functionality.");
        enable_edit_screen_percentage = false;
    }
    else
    {
        ValidateScreenPercentage(screen_percentage_address);
    }

    DOF::ScanDOFAddresses();

    spdlog::debug("Init done");
    return 1;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            this_module = module;
            if (auto main_thread = CreateThread(nullptr, 0, Main, nullptr, 0, nullptr))
            {
                SetThreadPriority(main_thread, THREAD_PRIORITY_HIGHEST);
                CloseHandle(main_thread);
            }
            if (auto thread = CreateThread(nullptr, 0, UpdatesBackgroundThread, nullptr, 0, nullptr))
            {
                CloseHandle(thread);
            }
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}
