#pragma once
#include "stdafx.h"
#include "globals.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

static void InitLogging()
{
    WCHAR buffer[MAX_PATH];
    GetModuleFileNameW(exe_module, buffer, MAX_PATH);
    module_path = std::filesystem::path(buffer);
    const auto game_path = std::filesystem::path(module_path).remove_filename();

    // Remove the existing log file when debugging.
    // Handy when testing patterns.
#   ifdef _DEBUG
    auto log_path = game_path / (dll_name + ".log");
    if (std::filesystem::exists(log_path))
    {
        std::filesystem::remove(log_path);
    }
#   endif

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("",
                                                                std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                                                                    game_path.native() + std::wstring(dll_name.begin(), dll_name.end()) + L".log",
                                                                    10 * 1024 * 1024, // 10MiB
                                                                    1
                                                                )));
    spdlog::flush_on(spdlog::level::debug);
}