#pragma once
#include "stdafx.h"
#include <mutex>
#include <queue>

static std::string dll_name = "PhotoModePatches";

static HMODULE this_module;
static HMODULE exe_module = GetModuleHandle(NULL);
static std::filesystem::path module_path;
static std::uint8_t* screen_percentage_address = nullptr;

static std::queue<int> queue_screen_percentage_updates;
static std::mutex queue_mutex;

static bool is_photo_mode_enabled = false;
static bool enable_screen_percentage_reset_on_close = true;
static bool enable_hud_detection = true;

static int screen_percentage_update_delay = 2000;
static int screen_percentage_default = 100;
static int screen_percentage_photos = 200;

static int key_percentage_photos = VK_F5;
static int key_percentage_default = VK_F6;
static int key_percentage_low_quality = VK_F7;

