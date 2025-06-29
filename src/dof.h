#pragma once
#include "pattern.h"
#include <spdlog/spdlog.h>

namespace DOF
{
    static std::uint8_t* address_dof_kernel_bg = nullptr;
    static std::uint8_t* address_dof_kernel_fg = nullptr;
    static std::uint8_t* address_dof_recombine = nullptr;
    static float initial_dof_kernel_fg = 0;
    static float initial_dof_kernel_bg = 0;
    static std::uint8_t initial_dof_recombine = 0;

    // first: foreground, second: background
    static std::tuple<std::uint8_t*,std::uint8_t*> FindDOFKernelMaxRadiusAddresses()
    {
        if (std::uint8_t* result = PatternScanHeap("72 00 2E 00 44 00 4F 00 46 00 2E 00 4B 00 65 00 72 00 6E 00 65 00 6C 00 2E 00 4D 00 61 00 78 00 46 00 6F 00 72 00 65 00 67 00 72 00 6F 00 75 00 6E 00 64 00 52 00 61 00 64 00 69 00 75 00 73 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 72 00 2E 00 44 00 4F 00 46 00 2E 00 4B 00 65 00 72 00 6E 00 65 00 6C 00 2E 00 4D 00 61 00 78 00 42 00 61 00 63 00 6B 00 67 00 72 00 6F 00 75 00 6E 00 64 00 52 00 61 00 64 00 69 00 75 00 73 00"))
        {
            auto foreground = result - 0x08;
            spdlog::info("Found address for DOF.Kernel.MaxForegroundRadius: 0x{:X}", reinterpret_cast<uintptr_t>(foreground));
            spdlog::debug("DOF.Kernel.MaxForegroundRadius values: {} & {}",
                          *reinterpret_cast<float*>(foreground),
                          *reinterpret_cast<float*>(foreground + 0x04)
            );
            initial_dof_kernel_fg = *reinterpret_cast<float*>(foreground);

            auto background = result + 0xA0 - 0x08;
            spdlog::info("Found address for DOF.Kernel.MaxBackgroundRadius: 0x{:X}", reinterpret_cast<uintptr_t>(background));
            spdlog::debug("DOF.Kernel.MaxBackgroundRadius values: {} & {}",
                          *reinterpret_cast<float*>(background),
                          *reinterpret_cast<float*>(background + 0x04)
            );
            initial_dof_kernel_bg = *reinterpret_cast<float*>(background);

            return std::make_tuple(foreground, background);
        }

        spdlog::error("Failed to find address for DOF.MaxRadius (Foreground & Background)!");
        return {};
    }

    static std::uint8_t* FindDOFRecombineQuality()
    {
        if (std::uint8_t* result = PatternScanHeap("72 00 2E 00 44 00 4F 00 46 00 2E 00 52 00 65 00 63 00 6F 00 6D 00 62 00 69 00 6E 00 65 00 2E 00 4D 00 69 00 6E 00 46 00 75 00 6C 00 6C 00 72 00 65 00 73 00 42 00 6C 00 75 00 72 00 52 00 61 00 64 00 69 00 75 00 73"))
        {
            result -= 0x54;
            spdlog::info("Found address for DOF.Recombine.Quality: 0x{:X}", reinterpret_cast<uintptr_t>(result));
            spdlog::debug("DOF.Recombine.Quality values: {} & {}",
                          *result,
                          *(result - 0x04)
            );
            initial_dof_recombine = *result;
            return result;
        } else
        {
            spdlog::error("Failed to find address for DOF.Recombine.Quality!");
            return {};
        }
    }

    static void ScanDOFAddresses()
    {
        address_dof_recombine = FindDOFRecombineQuality();

        const auto max_radius = FindDOFKernelMaxRadiusAddresses();
        address_dof_kernel_bg = std::get<0>(max_radius);
        address_dof_kernel_fg = std::get<1>(max_radius);
    }

    static void WriteDOFMaxRadius(float background, float foreground)
    {
        if (address_dof_kernel_bg != nullptr)
        {
            WritePattern(address_dof_kernel_bg, reinterpret_cast<std::uint8_t*>(&background), 4);
            WritePattern(address_dof_kernel_bg + 0x04, reinterpret_cast<std::uint8_t*>(&background), 4);
        } else
        {
            spdlog::warn("Did not write DOF.Kernel.MaxBackgroundRadius because the address is invalid");
        }

        if (address_dof_kernel_fg)
        {
            WritePattern(address_dof_kernel_fg, reinterpret_cast<std::uint8_t*>(&foreground), 4);
            WritePattern(address_dof_kernel_fg + 0x04, reinterpret_cast<std::uint8_t*>(&foreground), 4);
        } else
        {
            spdlog::warn("Did not write DOF.Kernel.MaxForegroundRadius because the address is invalid");
        }
    }

    static void WriteDOFMaxRadius(float value)
    {
        WriteDOFMaxRadius(value, value);
    }

    static void WriteDOFRecombine(std::uint8_t value)
    {
        if (address_dof_recombine)
        {
            WritePattern(address_dof_recombine, &value, 1);
            WritePattern(address_dof_recombine + 0x04, &value, 1);
        } else
        {
            spdlog::warn("Did not write DOF.Recombine because the address is invalid");
        }
    }
}
