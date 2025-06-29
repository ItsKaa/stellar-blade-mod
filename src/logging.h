#pragma once
#include "stdafx.h"
#include "globals.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

static std::filesystem::path GetLogPath()
{
    const auto game_path = std::filesystem::path(module_path).remove_filename();
    return game_path / (dll_name + ".log");
}

static void InitLogging()
{
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("",
                                                                std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                                                                    GetLogPath(),
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
    if (const auto log_path = GetLogPath();
        std::filesystem::exists(log_path))
    {
        std::filesystem::resize_file(log_path, 0);
    }
}