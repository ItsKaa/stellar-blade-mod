#include "stdafx.h"
#include "config.h"
#include "dof.h"
#include "globals.h"
#include "logging.h"
#include "photo_mode.h"
#include "screen_percentage.h"
#include <spdlog/spdlog.h>

static void HandlePhotoModeActivate()
{
    is_photo_mode_enabled = false;

    DOF::WriteDOFRecombine(0);
    //DOF::WriteDOFMaxRadius(0.015); // example
}

static void HandlePhotoModeDeactivate()
{
    is_photo_mode_enabled = false;
    if (enable_screen_percentage_reset_on_close)
    {
        spdlog::info("Photo mode deactivated, resetting screen percentage value to {:d}", screen_percentage_default);
        std::unique_lock lock(queue_mutex);
        queue_screen_percentage_updates.push(screen_percentage_default);
    }

    DOF::WriteDOFRecombine(DOF::initial_dof_recombine);
    DOF::WriteDOFMaxRadius(DOF::initial_dof_kernel_bg, DOF::initial_dof_kernel_fg);
}

DWORD WINAPI HandleScreenPercentageUpdates(void*)
{
    bool shift = false, ctrl = false, alt = false;
    using namespace std::literals::chrono_literals;
    while (true)
    {
        int value = 0;
        {
            std::unique_lock lock(queue_mutex);
            if (!queue_screen_percentage_updates.empty())
            {
                value = queue_screen_percentage_updates.front();
                queue_screen_percentage_updates.pop();
            }
        }
        if (value > 0)
        {
            WriteScreenPercentageFixLag(screen_percentage_address, value, value <= screen_percentage_default ? screen_percentage_update_delay : 0);
        }

        // Handle key inputs in here as well for now, since they're all related to screen percentage.
        shift = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? true : false;
        ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
        alt = GetAsyncKeyState(VK_MENU) & 0x8000 ? true : false;

        if (!shift && !ctrl && !alt)
        {
            if (::GetAsyncKeyState(key_percentage_photos) & 0x8000)
            {
                spdlog::info("Key pressed, updating screen percentage to photo mode: {:d}", screen_percentage_photos);
                WriteScreenPercentage(screen_percentage_address, screen_percentage_photos);
                Sleep(200);
            }
            else if (::GetAsyncKeyState(key_percentage_default) & 0x8000)
            {
                spdlog::info("Key pressed, updating screen percentage to default: {:d}", screen_percentage_photos);
                WriteScreenPercentageFixLag(screen_percentage_address, screen_percentage_default, screen_percentage_update_delay);
                Sleep(200);
            }
            else if (::GetAsyncKeyState(key_percentage_low_quality) & 0x8000)
            {
                spdlog::info("Key pressed, updating screen percentage to low quality: {:d}", screen_percentage_photos);
                WriteScreenPercentage(screen_percentage_address, 32);
                Sleep(200);
            }
        }
        std::this_thread::sleep_for(100ms);
    }

    return 1;
}

DWORD WINAPI Main(void*)
{
    InitLogging();
    spdlog::info("=== Patch Initialized ===");
    InitConfig();
    spdlog::info("Module Path: {:s}", module_path.string());
    spdlog::info("Module Address: 0x{:X}", reinterpret_cast<uintptr_t>(exe_module));

    if (enable_hud_detection)
    {
        spdlog::debug("Enabling HUD Detection");
        PhotoMode::HookPhotoModeHUDVisibility();
    }

    PhotoMode::HookSelfieModeActivate(&HandlePhotoModeActivate);
    PhotoMode::HookPhotoModeActivate(&HandlePhotoModeActivate);
    PhotoMode::HookPhotoModeDeactivate(&HandlePhotoModeDeactivate);

    if (screen_percentage_address = FindScreenPercentageAddress();
        screen_percentage_address == nullptr)
    {
        spdlog::error("Unable to find the screen percentage address. Disabled the screen percentage functionality.");
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
            if (auto thread = CreateThread(nullptr, 0, HandleScreenPercentageUpdates, nullptr, 0, nullptr))
            {
                CloseHandle(thread);
            }
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default:
            break;
    }
    return TRUE;
}
