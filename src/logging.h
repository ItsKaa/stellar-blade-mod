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

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("",
                                                                std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                                                                    game_path.native() + std::wstring(dll_name.begin(), dll_name.end()) + L".log",
                                                                    10 * 1024 * 1024, // 10MiB
                                                                    1
                                                                )));
    spdlog::flush_on(spdlog::level::debug);
}

static void LogStartupMessage()
{
    spdlog::info("=== Patch Initialized ===");
}

static void TruncateLogFile()
{
    const auto game_path = std::filesystem::path(module_path).remove_filename();
    if (const auto log_path = game_path / (dll_name + ".log"); std::filesystem::exists(log_path))
    {
        std::filesystem::resize_file(log_path, 0);
    }
}