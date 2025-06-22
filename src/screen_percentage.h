#pragma once
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>

static std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> CalculateScreenPercentage(const int value)
{
    std::uint8_t firstByte = 0x00;
    std::uint8_t secondByte = 0x00;
    std::uint8_t thirdByte = 0x42;

    if (value == 1)
    {
        secondByte = 0x80;
        thirdByte = 0x3F;
    }
    else if (value >= 2 && value <= 4)
    {
        secondByte = (value - 2) * 64;
    }
    else if (value >= 5 && value <= 7)
    {
        secondByte = 128 + (value-4) * 32;
    }
    else if (value >= 8 && value <= 16)
    {
        secondByte = (value - 8) * 16;
        thirdByte = 0x41;
    }
    else if (value >= 17 && value <= 31)
    {
        secondByte = 128 + (value-16) * 8;
        thirdByte = 0x41;
    }
    else if (value >= 32 && value <= 64)
    {
        secondByte = (value-32)*4;
    }
    else if (value >= 65 && value <= 127)
    {
        secondByte = value * 2;
    }
    else if (value >= 128 && value <= 256)
    {
        secondByte = value - 128;
        thirdByte = 0x43;
    }
    else if (value >= 257 && value <= 510)
    {
        firstByte = (value % 2 == 0) ? 0x00 : 0x80;
        secondByte = value / 2;
        thirdByte = 0x43;
    }
    else
    {
        // Unsupported value
        return {};
    }
    return {firstByte, secondByte, thirdByte};
}

static std::string ScreenPercentageBytesToPattern(const std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>& bytes)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase
            << std::setw(2) << std::setfill('0') << static_cast<int>(std::get<0>(bytes))
            << " "
            << std::setw(2) << std::setfill('0') << static_cast<int>(std::get<1>(bytes))
            << " "
            << std::setw(2) << std::setfill('0') << static_cast<int>(std::get<2>(bytes));
    return ss.str();
}

static std::uint8_t* FindScreenPercentageAddress()
{
	//if (std::uint8_t* result = PatternScanHeap("00 00 ?? ?? ?? 00 ?? ?? ?? 68 53 FA 45 01 00 00 00 ?? 5B"))
    if (std::uint8_t* result = PatternScanHeap("00 00 00 ?? ?? ?? 00 ?? ?? ?? 68 53 FA 45 01 00 00 ?? ?? 5B"))
    {
        spdlog::info("Found address for ScreenPercentage: 0x{:X}", reinterpret_cast<uintptr_t>(result));
        spdlog::info("ScreenPercentage values {:X} {:X} {:X} + {:X} {:X} {:X}",
            *(result+0x03), *(result+0x04), *(result+0x05),
            *(result+0x07), *(result+0x08), *(result+0x09)
        );
        return result + 0x03;
    }
    else
    {
        spdlog::error("Failed to find address for ScreenPercentage!");
        return {};
    }
}

static std::optional<std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>> ReadScreenPercentageBytes(const std::uint8_t* screen_percentage_address, const bool first_value = true)
{
    if (screen_percentage_address == nullptr)
        return {};

    int offset = 0;
    if (!first_value)
    {
        offset += 0x04;
    }

    const auto first  = *(screen_percentage_address + offset);
    const auto second  = *(screen_percentage_address + offset + 0x01);
    const auto third = *(screen_percentage_address + offset + 0x02);
    return {{first, second, third}};
}

static bool WriteScreenPercentage(std::uint8_t* screen_percentage_address, const int value)
{
    if (screen_percentage_address == nullptr)
    {
        spdlog::debug("Invalid address received for ScreenPercentage. Skipping.");
        return false;
    }

    // Calculate bytes
    const auto bytes = CalculateScreenPercentage(value);
    auto first_byte = std::get<0>(bytes);
    auto second_byte = std::get<1>(bytes);
    auto third_byte = std::get<2>(bytes);
    if (first_byte == 0 && second_byte == 0 && third_byte == 0)
    {
        spdlog::error("Screen percentage value got out of bounds or the calculation failed. Value: ({:d})", value);
        return false;
    }

    spdlog::info("Going to write {:X} {:X} {:X} {:X} {:X} {:X} {:X} to {:X}",
                 first_byte, second_byte, third_byte,
                 *(screen_percentage_address + 0x03),
                 first_byte, second_byte, third_byte,
                 reinterpret_cast<uintptr_t>(screen_percentage_address));

    //std::uint8_t bytes_to_write[7] = {0x00, 0x16, 0x43, 0x00, 0x00, 0x16, 0x43};
    const std::uint8_t bytes_to_write[7] = {
        first_byte, second_byte, third_byte,
        0x00,
        first_byte, second_byte, third_byte
    };

    if (!WritePattern(screen_percentage_address, bytes_to_write , sizeof(bytes_to_write)))
    {
        spdlog::error("Failed to write ScreenPercentage!");
        return false;
    }
    return true;
}

static bool WriteScreenPercentageFixLag(std::uint8_t* screen_percentage_address, const int value, int delay = 1000)
{
    // Write a low value first to correct a lag issue (guessing caused by streaming assets holding too much stuff in memory).
    if (WriteScreenPercentage(screen_percentage_address, 32))
    {
        // Wait a little and write the full value.
        if (delay > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        return WriteScreenPercentage(screen_percentage_address, value);
    }
    return false;
}