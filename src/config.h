#pragma once
#include "globals.h"
#include <filesystem>
#include <fstream>
#include <inipp.h>
#include <spdlog/spdlog.h>

static void InitConfig()
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
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_Default",  key_percentage_default);
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_Photos",  key_percentage_photos);
        inipp::get_value(ini.sections["Keys"], "ScreenPercentage_LowQuality",  key_percentage_low_quality);

        spdlog::info("Loaded configuration file");
        spdlog::info("Screen Percentage, Default: {}", screen_percentage_default);
        spdlog::info("Screen Percentage, Photos: {}", screen_percentage_photos);
        spdlog::info("Screen Percentage, Delay: {}", screen_percentage_update_delay);
        spdlog::info("HUD Detection: {}", enable_hud_detection);
        spdlog::info("Key: ScreenPercentage Default: {}", key_percentage_default);
        spdlog::info("Key: ScreenPercentage Photos: {}", key_percentage_photos);
        spdlog::info("Key: ScreenPercentage Low Quality: {}", key_percentage_low_quality);
    }
    else
    {
        spdlog::error("Failed to open config file! Please ensure it exists in: {:s}", ini_path.string());
    }
}
