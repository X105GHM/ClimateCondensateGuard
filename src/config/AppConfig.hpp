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

        static constexpr int kUltrasonicTrig = 17;
        static constexpr int kUltrasonicEcho = 21;
        static constexpr int kWaterSensor = 3;
        static constexpr int kPumpGate = 18;
        static constexpr int kClimateEnable = 15;
        static constexpr int kBuzzer = 43;
        static constexpr int kHallCurrentAdc = 2;
    };

    struct TankConfig
    {
        float sensorToBottomMm = 206.4F;
        float pumpOnLevelMm = 160.0F;
        float pumpOffLevelMm = 40.0F;
        float validMinMm = 25.0F;
        float validMaxMm = 206.4F;
    };

    struct WaterSensorConfig
    {
        bool activeHigh = true;
    };

    struct CurrentSensorConfig
    {
        float adcRefMv = 3300.0F;
        float adcMaxCounts = 4095.0F;
        float zeroCurrentMv = 1600.0F;
        float sensitivityMvPerA = 50.0F;
        float currentDetectedA = 0.2F;
    };

    struct PumpSafetyConfig
    {
        uint32_t noCurrentFaultDelayMs = 2500;
        uint32_t noDrainFaultDelayMs = 600000;
        float minRequiredLevelDropMm = 5.0F;
    };

    struct UltrasonicConfig
    {
        uint32_t echoTimeoutUs = 8000;
        bool filterEnabled = true;
        float filterAlpha = 0.25F;
        float maxAcceptedJumpMm = 60.0F;
        uint8_t maxInvalidHoldCount = 3;
    };

    struct EnergyConfig
    {
        bool enabled = true;
        IPAddress e3dcIp{192, 168, 178, 162};
        uint16_t modbusPort = 502;
        uint8_t modbusUnitId = 1;

        int32_t startExportThresholdW = 1500;
        int32_t stopImportThresholdW = 1500;
        uint16_t minBatterySocPercent = 25;
        uint32_t pollIntervalMs = 5000;
    };

    struct WebConfig
    {
        uint16_t port = 80;
    };

    struct OtaConfig
    {
        bool enabled = true;
        String username = "admin";
        String password = "av>zk55U=)CCgVeWpwX";
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
    inline constexpr UltrasonicConfig kUltrasonic {};
    inline const EnergyConfig kEnergy{};
    inline const ClimateNetworkConfig kClimateNet {};
    inline constexpr WebConfig kWeb{};
    inline const OtaConfig kOta {};
    inline constexpr TaskConfig kTask{};
    inline constexpr OperationMode kDefaultMode = OperationMode::Auto;
}
