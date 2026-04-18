#pragma once

#include <Arduino.h>
#include <cstdint>

enum class LogLevel : uint8_t
{
    Debug,
    Info,
    Warning,
    Error
};

enum class OperationMode : uint8_t
{
    Auto,
    ForcedOn,
    ForcedOff
};

enum FaultBits : uint32_t
{
    kFaultNone = 0U,
    kFaultOverflowSensor = 1U << 0,
    kFaultPumpNoCurrent = 1U << 1,
    kFaultPumpNoDrain = 1U << 2,
    kFaultUltrasonicInvalid = 1U << 3,
    kFaultEnergyOffline = 1U << 4
};

inline bool hasFault(const uint32_t mask, const FaultBits bit)
{
    return (mask & static_cast<uint32_t>(bit)) != 0U;
}

inline void setFault(uint32_t &mask, const FaultBits bit, const bool value = true)
{
    if (value)
    {
        mask |= static_cast<uint32_t>(bit);
    }
    else
    {
        mask &= ~static_cast<uint32_t>(bit);
    }
}

inline String toString(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Error:
        return "error";
    }
    return "unknown";
}

inline String toString(const OperationMode mode)
{
    switch (mode)
    {
    case OperationMode::Auto:
        return "auto";
    case OperationMode::ForcedOn:
        return "forced_on";
    case OperationMode::ForcedOff:
        return "forced_off";
    }
    return "unknown";
}
