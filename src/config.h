#pragma once
#include "globals.h"
#include "logging.h"
#include <filesystem>
#include <fstream>
#include <inipp.h>
#include <spdlog/spdlog.h>

template<typename T>
static bool ReadValue(inipp::Ini<char>& ini, const std::string& section_name, const std::string& key, T& dest)
{
    if (!inipp::get_value(ini.sections[section_name], key, dest))
    {
        spdlog::debug("Failed to read {:s}.{:s}", section_name, key);
        return false;
    }
    return true;
}

static bool ReadPreset(inipp::Ini<char>& ini, const std::string& name, const PresetType type)
{
    ConfigPreset preset;
    preset.name = name;
    preset.type = type;
    const auto section_name = "Presets." + name;

    if (!ReadValue(ini, section_name, "ScreenPercentage", preset.screen_percentage))
    {
        spdlog::debug("Screen percentage invalid for preset '{:s}'. Skipping.", name);
        return false;
    }
    ReadValue(ini, section_name, "Hotkey", preset.hotkey);
    ReadValue(ini, section_name, "DOF.Recombine", preset.dof_recombine);
    ReadValue(ini, section_name, "DOF.Foreground", preset.dof_max_foreground_radius);
    ReadValue(ini, section_name, "DOF.Background", preset.dof_max_background_radius);
    config_presets.push_back(std::move(preset));
    return true;
}

static ConfigPreset GetConfigPreset(PresetType type)
{
    if (const auto it = std::ranges::find_if(config_presets, [type](const ConfigPreset& preset) { return preset.type == type; });
        it != config_presets.end())
    {
        return *it;
    }

    spdlog::error("Failed to find config preset with type {}", static_cast<int>(type));
    return {};
}

static ConfigPreset GetConfigPresetByName(const std::string& name)
{
    if (const auto it = std::ranges::find_if(config_presets, [&name](const ConfigPreset& preset) { return preset.name == name; });
        it != config_presets.end())
    {
        return *it;
    }
    return {};
}

static std::filesystem::path GetIniPath()
{
    const auto game_path = std::filesystem::path(module_path).remove_filename();
    return game_path / (std::string(dll_name) + ".ini");
}

static void InitConfig(bool reloading = false)
{
    const auto ini_path = GetIniPath();
    if (std::ifstream ini_stream(ini_path);
        ini_stream.is_open())
    {
        inipp::Ini<char> ini;
        ini.parse(ini_stream);
        ini.strip_trailing_comments();

        if (!reloading)
        {
            bool clear_log = false;
            inipp::get_value(ini.sections["Logging"], "Truncate",  clear_log);
            if (clear_log)
            {
                TruncateLogFile();
                LogStartupMessage();
                spdlog::info("Wiped log started");
            }
        }

        bool debug_logging = false;
        inipp::get_value(ini.sections["Logging"], "Debug",  debug_logging);
        spdlog::set_level(debug_logging ? spdlog::level::debug : spdlog::level::info);
        if (!reloading)
        {
            spdlog::debug("Debug logging enabled");
        }

        inipp::get_value(ini.sections["General"], "EnableReload",  enable_reload_config);
        inipp::get_value(ini.sections["General"], "ReloadCheckDelay",  reload_check_delay_seconds);
        inipp::get_value(ini.sections["General"], "ReloadReactivatesPreset",  reload_reactivates_preset);
        inipp::get_value(ini.sections["General"], "AutoSwapToDefaultPreset",  enable_reset_to_default);
        inipp::get_value(ini.sections["Screen Percentage"], "Enabled",  enable_edit_screen_percentage);
        inipp::get_value(ini.sections["Screen Percentage"], "Delay",  screen_percentage_update_delay);
        inipp::get_value(ini.sections["Depth of Field"], "Enabled",  enable_edit_dof);
        inipp::get_value(ini.sections["PhotoMode HUD Detection"], "Enabled",  enable_hud_detection);

        config_presets.clear();
        ReadPreset(ini, "Default", PresetType::Default);
        ReadPreset(ini, "Photos", PresetType::PhotoMode);
        for (int i = 1; i < 100; ++i)
        {
            if (!ReadPreset(ini, std::to_string(i), PresetType::UserDefined))
            {
                break;
            }
        }

        // For faster access:
        config_preset_default = GetConfigPreset(PresetType::Default);
        config_preset_photos = GetConfigPreset(PresetType::PhotoMode);
        config_preset_hotkeys.clear();
        for (const auto& preset : config_presets)
        {
            config_preset_hotkeys.emplace(preset.hotkey);
        }

        spdlog::info("Loaded configuration file");
        spdlog::info("Screen Percentage, Delay: {}", screen_percentage_update_delay);
        spdlog::info("HUD Detection: {}", enable_hud_detection);
        for (auto& preset : config_presets)
        {
            preset.LogInfo();
        }
    }
    else
    {
        spdlog::error("Failed to open config file! Please ensure it exists in: {:s}", ini_path.string());
    }
}

static bool ReloadConfigIfNeeded()
{
    if (!enable_reload_config)
    {
        return false;
    }

    const auto clock = std::chrono::system_clock::now();
    if (config_last_check == std::chrono::time_point<std::chrono::system_clock>{})
    {
        // wait for the next cycle.
        config_last_check = clock;
        return false;
    }

    if (const auto check_diff = clock - config_last_check;
        check_diff <= std::chrono::seconds(reload_check_delay_seconds))
    {
        return false;
    }

    config_last_check = clock;
    if (const auto ini_path = GetIniPath();
        std::filesystem::exists(ini_path))
    {
        const auto mtime = std::filesystem::last_write_time(ini_path);
        if (config_last_modification_date == std::filesystem::file_time_type{})
        {
            // wait for the next cycle.
            config_last_modification_date = mtime;
            return false;
        }

        if (const auto sec = std::chrono::duration_cast<std::chrono::seconds>(mtime - config_last_modification_date);
            sec >= std::chrono::seconds(1)) // just to be sure.
        {
            std::stringstream ss;
            ss << mtime;
            spdlog::info("Changes detected, reloading config... (mtime: {})", ss.str());
            config_last_modification_date = mtime;
            InitConfig(true);
            return true;
        }
    }
    else
    {
        spdlog::debug("Failed to read config file");
    }
    return false;
}
