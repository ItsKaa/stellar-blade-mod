#pragma once
#include "stdafx.h"
#include "config_preset.h"
#include <mutex>
#include <queue>
#include <set>

static std::string dll_name = "PhotoModePatches";

static HMODULE this_module;
static HMODULE exe_module = GetModuleHandle(NULL);
static std::filesystem::path module_path;

// Addresses:
static std::uint8_t* screen_percentage_address = nullptr;

// Stored values from address scanning/hook callbacks:
static bool is_photo_mode_enabled = false;

// Queue for modifications in background thread (required for the sleep):
static std::queue<int> queue_screen_percentage_updates;
static std::mutex queue_mutex;

// Config related:
static std::vector<ConfigPreset> config_presets;
static std::set<int> config_preset_hotkeys;
static ConfigPreset config_preset_default;
static ConfigPreset config_preset_photos;

static int screen_percentage_update_delay = 2000;
static bool enable_edit_screen_percentage = false;
static bool enable_edit_dof = false;
static bool enable_hud_detection = true;
static bool enable_reset_to_default = true;