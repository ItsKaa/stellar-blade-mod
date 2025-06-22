#include "stdafx.h"
#include "pattern.h"
#include "screen_percentage.h"
#include <string>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <safetyhook.hpp>

std::string dll_name = "PhotoModePatches";

HMODULE this_module;
HMODULE exe_module = GetModuleHandle(NULL);
std::filesystem::path module_path;
std::uint8_t* screen_percentage_address = nullptr;

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
            }
        });
    }
    else
    {
        spdlog::error("Failed to find the address for game unfreeze!");
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

    if (screen_percentage_address = FindScreenPercentageAddress();
        screen_percentage_address == nullptr)
    {
        spdlog::error("Unable to find the screen percentage address. Disabled the screen percentage functionality.");
    }
    else
    {
        const auto screen_percentage_bytes_1 = ReadScreenPercentageBytes(screen_percentage_address, true);
        const auto screen_percentage_bytes_2 = ReadScreenPercentageBytes(screen_percentage_address, false);
        if (!screen_percentage_bytes_1.has_value() || !screen_percentage_bytes_2.has_value())
        {
            spdlog::error("Unable to read the screen percentage. Disabled the screen percentage functionality.");
            screen_percentage_address = nullptr;
        }
        else
        {
            const auto first_byte_1 = std::get<0>(screen_percentage_bytes_1.value());
            const auto second_byte_1 = std::get<1>(screen_percentage_bytes_1.value());
            const auto third_byte_1 = std::get<2>(screen_percentage_bytes_1.value());

            const auto first_byte_2 = std::get<0>(screen_percentage_bytes_2.value());
            const auto second_byte_2 = std::get<1>(screen_percentage_bytes_2.value());
            const auto third_byte_2 = std::get<2>(screen_percentage_bytes_2.value());

            spdlog::debug("Initial ScreenPercentage bytes (1): {:X} {:X} {:X}", first_byte_1, second_byte_1, third_byte_1);
            spdlog::debug("Initial ScreenPercentage bytes (2): {:X} {:X} {:X}", first_byte_2, second_byte_2, third_byte_2);

            if (first_byte_1 == 0x00 && second_byte_1 == 0xC8 && third_byte_1 == 0x42
                && first_byte_2 == 0x00 && second_byte_2 == 0xC8 && third_byte_2 == 0x42
            )
            {
                spdlog::info("ScreenPercentage is the expected value (100)");
            }
            else
            {
                if (first_byte_1 != 0x00 || second_byte_1 != 0xC8 || third_byte_1 != 0x42)
                {
                    spdlog::warn("Screen percentage is not the expected value of 100, the pointer (1) may be incorrect and could lead to crashes.");
                }
                if (first_byte_2 != 0x00 || second_byte_2 != 0xC8 || third_byte_2 != 0x42)
                {
                    spdlog::warn("Screen percentage is not the expected value of 100, the pointer (2) may be incorrect and could lead to crashes.");
                }
                if (first_byte_1 != first_byte_2 || second_byte_1 != second_byte_2 || third_byte_1 != third_byte_2)
                {
                    spdlog::error("Unexpected values encountered for screen percentage. {:X} {:X} {:X} != {:X} {:X} {:X}. Disabled the screen percentage functionality.",
                        first_byte_1, second_byte_1, third_byte_1,
                        first_byte_2, second_byte_2, third_byte_2
                    );
                    screen_percentage_address = nullptr;
                }
            }
        }
    }

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
