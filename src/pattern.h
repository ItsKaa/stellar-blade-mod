#pragma once
#include "stdafx.h"
#include <cstring>

static std::vector<int> PatternToBytes(const char* pattern)
{
    auto bytes = std::vector<int>{};
    const auto start = const_cast<char*>(pattern);
    const auto end = const_cast<char*>(pattern) + strlen(pattern);

    for (auto current = start; current < end; ++current)
    {
        if (*current == '?')
        {
            ++current;
            if (*current == '?')
                ++current;
            bytes.push_back(-1);
        } else
        {
            bytes.push_back(static_cast<int>(strtoul(current, &current, 16)));
        }
    }
    return bytes;
}

static std::vector<std::uint8_t*> PatternScanRegion(std::uint8_t* region_start, std::size_t region_size, const std::vector<int>& pattern_bytes, bool single = true)
{
    std::vector<std::uint8_t*> results;
    const auto s = pattern_bytes.size();
    const auto d = pattern_bytes.data();

    for (std::size_t i = 0; i < region_size - s; ++i)
    {
        bool found = true;
        for (std::size_t j = 0; j < s; ++j)
        {
            if (d[j] != -1 && region_start[i + j] != d[j])
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            results.emplace_back(&region_start[i]);
            if (single)
                return results;
        }
    }

    return results;
}

static std::vector<std::uint8_t*> PatternScan(void* module, const char* pattern, bool single)
{
    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)(static_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);

    auto size = ntHeaders->OptionalHeader.SizeOfImage;
    const auto bytes = PatternToBytes(pattern);
    return PatternScanRegion(static_cast<std::uint8_t*>(module), size, bytes, single);
}

static std::uint8_t* PatternScan(void* module, const char* pattern)
{
    return PatternScan(module, pattern, true)[0];
}

static std::vector<std::uint8_t*> PatternScanHeap(const char* pattern, bool single)
{
    std::vector<std::uint8_t*> results;
    const auto pattern_bytes = PatternToBytes(pattern);
    SYSTEM_INFO system_info{0};
    GetSystemInfo(&system_info);

    const auto* address_end = static_cast<std::uint8_t*>(system_info.lpMaximumApplicationAddress);
    std::uint8_t* current_address = nullptr;
    SIZE_T bytes_read = 0;

    while (current_address < address_end)
    {
        MEMORY_BASIC_INFORMATION mbi;
        HANDLE process_handle = GetCurrentProcess();
        if (!VirtualQueryEx(process_handle, current_address, &mbi, sizeof(mbi)))
            return results;

        std::unique_ptr<std::uint8_t> buffer;
        if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS)
        {
            buffer.reset(new std::uint8_t[mbi.RegionSize]);
            DWORD protect;
            if (VirtualProtectEx(process_handle, mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &protect))
            {
                ReadProcessMemory(process_handle, mbi.BaseAddress, buffer.get(), mbi.RegionSize, &bytes_read);
                VirtualProtectEx(process_handle, mbi.BaseAddress, mbi.RegionSize, protect, &protect);
                if (auto region_results = PatternScanRegion(buffer.get(), bytes_read, pattern_bytes);
                    !region_results.empty())
                {
                    // Correct the pointers.
                    for (auto result : region_results)
                    {
                        results.emplace_back(current_address + (result - buffer.get()));
                        if (single)
                        {
                            return results;
                        }
                    }
                }
            }
        }
        current_address = current_address + mbi.RegionSize;
    }
    return results;
}

static std::uint8_t* PatternScanHeap(const char* pattern)
{
    return PatternScanHeap(pattern, true)[0];
}

static bool WritePattern(std::uint8_t* address, const std::uint8_t* values, unsigned int size)
{
    DWORD protect;
    if (VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &protect))
    {
        memcpy((LPVOID)address, values, size);
        VirtualProtect((LPVOID)address, size, protect, &protect);
        return true;
    }
    return false;
}
