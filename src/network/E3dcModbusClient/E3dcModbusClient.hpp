#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include "core/Logger/Logger.hpp"

struct E3dcTelemetry
{
    bool valid{false};
    int32_t pvPowerW{0};
    int32_t batteryPowerW{0};
    int32_t housePowerW{0};
    int32_t gridPointPowerW{0};
    uint16_t batterySocPercent{0};
    uint16_t emsStatus{0};
};

class E3dcModbusClient
{
public:
    explicit E3dcModbusClient(Logger &logger);

    bool poll(const IPAddress &ip, uint16_t port, uint8_t unitId, E3dcTelemetry &telemetryOut);

private:
    bool readHoldingRegisters(const IPAddress &ip, uint16_t port, uint8_t unitId, uint16_t startRegister, uint16_t count, uint8_t *buffer, size_t bufferLen);
    static int32_t parseInt32BE(const uint8_t *data);
    static uint16_t parseUInt16BE(const uint8_t *data);

    Logger &logger_;
    uint16_t transactionId_{0};
};
