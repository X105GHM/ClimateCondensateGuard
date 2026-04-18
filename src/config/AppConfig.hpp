#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include "core/Types.hpp"

namespace config
{
    struct Pins
    {
        static constexpr int kEthMiso = 12;
        static constexpr int kEthMosi = 11;
        static constexpr int kEthSclk = 13;
        static constexpr int kEthCs = 14;
        static constexpr int kEthRst = 9;
        static constexpr int kEthInt = 10;

        static constexpr int kUltrasonicTrig = 4;
        static constexpr int kUltrasonicEcho = 5;
        static constexpr int kWaterSensor = 6;
        static constexpr int kPumpGate = 7;
        static constexpr int kClimateEnable = 15;
        static constexpr int kBuzzer = 16;
        static constexpr int kHallCurrentAdc = 1;
    };

    struct TankConfig
    {
        float sensorToBottomMm = 220.0F;
        float pumpOnLevelMm = 120.0F;
        float pumpOffLevelMm = 40.0F;
        float validMinMm = 20.0F;
        float validMaxMm = 220.0F;
    };

    struct WaterSensorConfig
    {
        bool activeHigh = true;
    };

    struct CurrentSensorConfig
    {
        float adcRefMv = 3300.0F;
        float adcMaxCounts = 4095.0F;
        float zeroCurrentMv = 1650.0F;
        float sensitivityMvPerA = 100.0F;
        float currentDetectedA = 0.25F;
    };

    struct PumpSafetyConfig
    {
        uint32_t noCurrentFaultDelayMs = 2500;
        uint32_t noDrainFaultDelayMs = 12000;
        float minRequiredLevelDropMm = 12.0F;
    };

    struct EnergyConfig
    {
        bool enabled = true;
        IPAddress e3dcIp{192, 168, 178, 162};
        uint16_t modbusPort = 502;
        uint8_t modbusUnitId = 1;

        int32_t startExportThresholdW = 250;
        int32_t stopImportThresholdW = 50;
        uint16_t minBatterySocPercent = 25;
        uint32_t pollIntervalMs = 5000;
    };

    struct WebConfig
    {
        uint16_t port = 80;
    };

    struct TaskConfig
    {
        uint32_t sensorTaskMs = 250;
        uint32_t controlTaskMs = 250;
        uint32_t buzzerTaskMs = 80;
        uint32_t webTaskMs = 10;
        uint32_t logTaskMs = 200;
    };

    struct ClimateNetworkConfig
    {
        bool enabled = true;
        bool useLocalPin = false;
        String baseUrl = "http://192.168.178.188:8080"; // Debian server running homebridge with homebridge-http plugin
        String bearerToken = "SmartWaterPumpBridge2026";

        uint32_t timeoutMs = 4000;
        uint32_t resyncMs = 30000;

        String mode = "cool";
        String fan = "auto";
        uint8_t targetTempC = 24;
    };

    inline constexpr TankConfig kTank{};
    inline constexpr WaterSensorConfig kWater{};
    inline constexpr CurrentSensorConfig kCurrent{};
    inline constexpr PumpSafetyConfig kPumpSafety{};
    inline const EnergyConfig kEnergy{};
    inline const ClimateNetworkConfig kClimateNet {};
    inline constexpr WebConfig kWeb{};
    inline constexpr TaskConfig kTask{};
    inline constexpr OperationMode kDefaultMode = OperationMode::Auto;
}
