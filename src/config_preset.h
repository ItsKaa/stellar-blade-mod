#pragma once
#include <cmath>
#include <spdlog/spdlog.h>

enum class PresetType
{
    Unknown,
    Default,
    PhotoMode,
    UserDefined,
};

struct ConfigPreset
{
    std::string name = "none";
    PresetType type = PresetType::Unknown;
    int hotkey = -1;
    int screen_percentage = 100;
    int dof_recombine = -1;
    float dof_max_foreground_radius = NAN;
    float dof_max_background_radius = NAN;

    void LogInfo()
    {
        spdlog::info("Preset {}: Hotkey: {}, ScreenPercentage: {}, DOF.Recombine: {}, DOF.Foreground: {}, DOF.Background: {}",
            name,
            hotkey == -1 ? "unused" : std::to_string(hotkey),
            screen_percentage,
            dof_recombine == -1 ? "unused" : std::to_string(dof_recombine),
            std::isnan(dof_max_foreground_radius) ? "unused" : std::to_string(dof_max_foreground_radius),
            std::isnan(dof_max_background_radius) ? "unused" : std::to_string(dof_max_background_radius)
        );
    }
};