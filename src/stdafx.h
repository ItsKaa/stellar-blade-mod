#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define SPDLOG_WCHAR_FILENAMES

#ifdef _DEBUG
#   define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#   define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <windows.h>
#include <stdio.h>
#include <filesystem>
#include <vector>
