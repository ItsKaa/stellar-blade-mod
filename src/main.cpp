#include "stdafx.h"
#include "pattern.h"
#include "screen_percentage.h"
#include <string>
#include <fstream>
#include <queue>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <safetyhook.hpp>

std::string dll_name = "PhotoModePatches";

HMODULE this_module;
HMODULE exe_module = GetModuleHandle(NULL);
std::filesystem::path module_path;
std::uint8_t* screen_percentage_address = nullptr;

std::queue<int> queue_screen_percentage_updates;
std::mutex queue_mutex;
int screen_percentage_update_delay = 2000;
int screen_percentage_default = 100;
int screen_percentage_photos = 200;
bool is_game_paused = false;
bool restore_screen_percentage_on_unfreeze = false;

void LogAllValues(const char* title, const safetyhook::Context& ctx)
{
    spdlog::debug(
        "{:s}, values: {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}"
        , title
        , ctx.rax, ctx.rbx, ctx.rcx, ctx.rdx, ctx.rsi, ctx.rdi, ctx.rbp
        , ctx.rsp, ctx.r8, ctx.r9, ctx.r10, ctx.r11, ctx.r12, ctx.r13, ctx.r14, ctx.r15, ctx.rip
    );
}

void HookTemporalAASamples()
{
    if (std::uint8_t* result = PatternScan(exe_module, "8B ?? 18 ?? 8B ?? ?? 8B ?? 10 89 ?? 48 ?? ??"))
    {
        spdlog::info("Found address for TemporalAASamples: 0x{:X}", reinterpret_cast<uintptr_t>(result));

        static SafetyHookMid midHook{};
        midHook = safetyhook::create_mid(result + 0x0A, [](safetyhook::Context& ctx) {
            if (ctx.rax == 2)
            {
                spdlog::info("Found TemporalAASamples value of {:d}", ctx.rax);
                LogAllValues("TemporalAASamples", ctx);
            }
        });
    } else
    {
        spdlog::error("Failed to find address for TemporalAASamples!");
    }
}

void HookGameFreeze()
{
    if (std::uint8_t* result = PatternScan(exe_module, "89 86 60 03 00 00 ?? ?? 64 03 00 00 ?? ?? 48 ?? ?? 58 03 00 00"))
    {
        spdlog::info("Found address for game freeze: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid midHook;
        midHook = safetyhook::create_mid(result, [](safetyhook::Context& ctx) {
            LogAllValues("game freeze", ctx);

            if (ctx.rax == 1
                && ctx.r12 == 1 && ctx.r15 == 1)
            {
                spdlog::info("Detected game freeze.");
                is_game_paused = true;
            }
        });
    } else
    {
        spdlog::error("Failed to find address for game freeze!");
    }
}

void HookGameUnfreeze()
{
    if (std::uint8_t* result = PatternScan(exe_module, "44 29 76 08 48 83 C4 28 41 5E 5E C3"))
    {
        spdlog::info("Found address for game unfreeze: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid midHook{};
        midHook = safetyhook::create_mid(result, [](safetyhook::Context& ctx) {
            LogAllValues("game unfreeze", ctx);

            if (ctx.rax == 1
                && ctx.rdx == 1 && ctx.r14 == 1 && ctx.r15 == 1
                && ctx.rcx == 0 && ctx.rdi == 0 && ctx.r13 == 0)
            {
                spdlog::info("Detected game unfreeze.");
                is_game_paused = false;
                if (restore_screen_percentage_on_unfreeze)
                {
                    queue_screen_percentage_updates.push(screen_percentage_default);
                }
            }
        });
    }
    else
    {
        spdlog::error("Failed to find the address for game unfreeze!");
    }
}

void HookPhotoModeHUDVisibility()
{
    if (std::uint8_t* result = PatternScan(exe_module, "0F 94 ?? 33 ?? 88 81 ?? ?? ?? ?? 48 ?? ?? C7"))
    {
        spdlog::info("Found address for PhotoMode HUD Visibility: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid hook = safetyhook::create_mid(result + 0x05, [](safetyhook::Context& ctx) {
            if ((ctx.rdx == 1 || ctx.rdx == 0) && ctx.rbx == 0 )
            {
                const auto hud_visible = ctx.rdx == 1;
                spdlog::debug("Found HUD toggle {:s}", hud_visible ? "true" : "false");
                LogAllValues("HUD Visibility", ctx);

                if (!is_game_paused)
                {
                    spdlog::warn("Detected HUD toggle but game is not paused. Aborting.");
                    return;
                }

                std::unique_lock lock(queue_mutex);
                if (hud_visible)
                {
                    spdlog::info("HUD visible, resetting screen percentage value to {:d}", screen_percentage_default);
                    queue_screen_percentage_updates.push(screen_percentage_default);
                }
                else
                {
                    spdlog::info("HUD hidden, updating screen percentage value to {:d}", screen_percentage_photos);
                    queue_screen_percentage_updates.push(screen_percentage_photos);
                }
            }
        });
    }
    else
    {
        spdlog::error("Failed to find address for HUD Visibility!");
    }
}

void InitLogging()
{
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(exe_module, buffer, MAX_PATH);
    module_path = std::filesystem::path(buffer);
    const auto game_path = std::filesystem::path(module_path).remove_filename();

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("",
                                                                std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                                                                    game_path.native() + std::wstring(dll_name.begin(), dll_name.end()) + L".log",
                                                                    10 * 1024 * 1024, // 10MiB
                                                                    1
                                                                )));
    spdlog::flush_on(spdlog::level::debug);
}

DWORD WINAPI Main(void*)
{
    InitLogging();
    spdlog::info("=== Patch Initialized ===");
    spdlog::info("Module Path: {:s}", module_path.string());
    spdlog::info("Module Address: 0x{:X}", reinterpret_cast<uintptr_t>(exe_module));

    HookTemporalAASamples();
    HookGameFreeze();
    HookGameUnfreeze();
    HookPhotoModeHUDVisibility();

    if (screen_percentage_address = FindScreenPercentageAddress();
        screen_percentage_address == nullptr)
    {
        spdlog::error("Unable to find the screen percentage address. Disabled the screen percentage functionality.");
    }
    else
    {
        ValidateScreenPercentage(screen_percentage_address);
    }

    spdlog::debug("Init done");
    return 1;
}

DWORD WINAPI HandleScreenPercentageUpdates(void*)
{
    using namespace std::literals::chrono_literals;
    while (true)
    {
        std::unique_lock lock(queue_mutex);
        int value = 0;
        if (!queue_screen_percentage_updates.empty())
        {
            value = queue_screen_percentage_updates.front();
            queue_screen_percentage_updates.pop();
        }
        if (value > 0)
        {
            WriteScreenPercentageFixLag(screen_percentage_address, value, value <= screen_percentage_default ? screen_percentage_update_delay : 0);
        }

        std::this_thread::sleep_for(100ms);
    }

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
