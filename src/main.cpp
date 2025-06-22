#include <fstream>

#include "stdafx.h"
#include "pattern.h"
#include "screen_percentage.h"
#include <string>
#include <queue>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <safetyhook.hpp>
#include <inipp.h>

std::string dll_name = "PhotoModePatches";

HMODULE this_module;
HMODULE exe_module = GetModuleHandle(NULL);
std::filesystem::path module_path;
std::uint8_t* screen_percentage_address = nullptr;

std::queue<int> queue_screen_percentage_updates;
std::mutex queue_mutex;

bool enable_hud_detection = true;
bool enable_pause_detection = true;

int screen_percentage_update_delay = 2000;
int screen_percentage_default = 100;
int screen_percentage_photos = 200;
bool is_game_paused = false;

int key_percentage_photos = VK_F5;
int key_percentage_default = VK_F6;
int key_percentage_low_quality = VK_F7;


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
                spdlog::debug("Found TemporalAASamples value of {:d}", ctx.rax);
                LogAllValues("TemporalAASamples", ctx);
            }
        });
    } else
    {
        spdlog::error("Failed to find address for TemporalAASamples!");
    }
}

void HookGamePause()
{
    if (std::uint8_t* result = PatternScan(exe_module, "89 86 60 03 00 00 ?? ?? 64 03 00 00 ?? ?? 48 ?? ?? 58 03 00 00"))
    {
        spdlog::info("Found address for game pause: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid midHook;
        midHook = safetyhook::create_mid(result, [](safetyhook::Context& ctx) {
            LogAllValues("game pause", ctx);

            if (ctx.rax == 1
                && ctx.r12 == 1 && ctx.r15 == 1)
            {
                spdlog::debug("Detected game pause.");
                is_game_paused = true;
            }
        });
    } else
    {
        spdlog::error("Failed to find address for game pause!");
    }
}

void HookGameUnpause()
{
    if (std::uint8_t* result = PatternScan(exe_module, "44 29 76 08 48 83 C4 28 41 5E 5E C3"))
    {
        spdlog::info("Found address for game unpause: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        static SafetyHookMid midHook{};
        midHook = safetyhook::create_mid(result, [](safetyhook::Context& ctx) {
            LogAllValues("game unpause", ctx);

            if (ctx.rax == 1
                && ctx.rdx == 1 && ctx.r14 == 1 && ctx.r15 == 1
                && ctx.rcx == 0 && ctx.rdi == 0 && ctx.r13 == 0)
            {
                spdlog::debug("Detected game unpause.");
                is_game_paused = false;
                if (enable_pause_detection)
                {
                    spdlog::info("Game unpaused, resetting screen percentage value to {:d}", screen_percentage_default);
                    queue_screen_percentage_updates.push(screen_percentage_default);
                }
            }
        });
    }
    else
    {
        spdlog::error("Failed to find the address for game unpause!");
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

                // Selfie mode does not pause the game.
                // if (enable_pause_detection && !is_game_paused)
                // {
                //     spdlog::debug("Detected HUD toggle but game is not paused.. Aborting");
                //     return;
                // }

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

void InitConfig()
{
    const auto game_path = std::filesystem::path(module_path).remove_filename();
    const auto ini_path = game_path / (std::string(dll_name) + ".ini");
    if (std::ifstream ini_stream(ini_path);
        ini_stream.is_open())
    {
        inipp::Ini<char> ini;
        ini.parse(ini_stream);
        ini.strip_trailing_comments();

        bool debug_logging = false;
        inipp::get_value(ini.sections["Logging"], "Debug",  debug_logging);
        if (debug_logging)
        {
            spdlog::set_level(spdlog::level::debug);
            spdlog::default_logger()->set_level(spdlog::level::debug);
            spdlog::debug("Debug logging enabled");
        }

        inipp::get_value(ini.sections["Screen Percentage"], "Default",  screen_percentage_default);
        inipp::get_value(ini.sections["Screen Percentage"], "Photos",  screen_percentage_photos);
        inipp::get_value(ini.sections["Screen Percentage"], "Delay",  screen_percentage_update_delay);
        inipp::get_value(ini.sections["PhotoMode HUD Detection"], "Enabled",  enable_hud_detection);
        inipp::get_value(ini.sections["Pause Detection"], "Enabled",  enable_pause_detection);
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_Default",  key_percentage_default);
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_Photos",  key_percentage_photos);
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_LowQuality",  key_percentage_low_quality);

        spdlog::info("Loaded configuration file");
        spdlog::info("Screen Percentage, Default: {}", screen_percentage_default);
        spdlog::info("Screen Percentage, Photos: {}", screen_percentage_photos);
        spdlog::info("Screen Percentage, Delay: {}", screen_percentage_update_delay);
        spdlog::info("HUD Detection: {}", enable_hud_detection);
        spdlog::info("Pause Detection: {}", enable_pause_detection);
        spdlog::info("Key: ScreenPercentage Default: {}", key_percentage_default);
        spdlog::info("Key: ScreenPercentage Photos: {}", key_percentage_photos);
        spdlog::info("Key: ScreenPercentage Low Quality: {}", key_percentage_low_quality);
    }
    else
    {
        spdlog::error("Failed to open config file! Please ensure it exists in: {:s}", ini_path.string());
    }
}


DWORD WINAPI Main(void*)
{
    InitLogging();
    spdlog::info("=== Patch Initialized ===");
    InitConfig();
    spdlog::info("Module Path: {:s}", module_path.string());
    spdlog::info("Module Address: 0x{:X}", reinterpret_cast<uintptr_t>(exe_module));

    // HookTemporalAASamples(); // unused
    if (enable_pause_detection)
    {
        spdlog::debug("Enabling Pause Detection");
        HookGamePause();
        HookGameUnpause();
    }

    if (enable_hud_detection)
    {
        spdlog::debug("Enabling HUD Detection");
        HookPhotoModeHUDVisibility();
    }

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
