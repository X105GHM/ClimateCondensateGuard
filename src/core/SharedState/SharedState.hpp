#pragma once

#include <Arduino.h>
#include <functional>
#include "core/Types.hpp"

struct SensorState
{
    bool ultrasonicValid{false};
    float distanceMm{0.0F};
    float levelMm{0.0F};
    bool overflowSensorWet{false};
    float pumpCurrentA{0.0F};
    bool pumpCurrentDetected{false};
    uint32_t lastUpdateMs{0};
};

struct EnergyState
{
    bool valid{false};
    bool connected{false};
    int32_t pvPowerW{0};
    int32_t batteryPowerW{0};
    int32_t housePowerW{0};
    int32_t gridPointPowerW{0};
    uint16_t batterySocPercent{0};
    uint16_t emsStatus{0};
    bool coolingAllowedAuto{false};
    uint32_t lastUpdateMs{0};
};

struct ActuatorState
{
    bool pumpEnabled{false};
    bool climateEnabled{false};
    bool buzzerEnabled{false};
};

struct ClimateNetworkState 
{
    bool enabled {false};
    bool online {false};
    uint32_t lastCommandMs {0};
    String lastError;
};

struct SystemStateData
{
    SensorState sensors;
    EnergyState energy;
    ActuatorState actuators;
    ClimateNetworkState climateNetwork;
    OperationMode mode {OperationMode::Auto};
    uint32_t faultMask {0};
    bool ethernetConnected {false};
    String ethernetIp;
};

class SharedState
{
public:
    SharedState();
    ~SharedState();

    [[nodiscard]] SystemStateData snapshot() const;
    void update(const std::function<void(SystemStateData &)> &fn);

private:
    mutable SemaphoreHandle_t mutex_{nullptr};
    SystemStateData data_;
};
