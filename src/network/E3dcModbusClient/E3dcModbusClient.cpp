#include "E3dcModbusClient.hpp"

#include <NetworkClient.h>

E3dcModbusClient::E3dcModbusClient(Logger &logger) : logger_(logger){}

bool E3dcModbusClient::poll(const IPAddress &ip, const uint16_t port, const uint8_t unitId, E3dcTelemetry &telemetryOut)
{
    constexpr uint16_t kStartRegister = static_cast<uint16_t>(40068U - 40001U);
    constexpr uint16_t kRegisterCount = 18;

    uint8_t payload[kRegisterCount * 2]{};

    const bool ok = readHoldingRegisters
    (
        ip,
        port,
        unitId,
        kStartRegister,
        kRegisterCount,
        payload,
        sizeof(payload)
    );

    if (!ok)
    {
        telemetryOut.valid = false;
        return false;
    }

    telemetryOut.valid = true;

    telemetryOut.pvPowerW = parseInt32BE(&payload[0]);          // 40068 / 40069
    telemetryOut.batteryPowerW = parseInt32BE(&payload[4]);     // 40070 / 40071
    telemetryOut.housePowerW = parseInt32BE(&payload[8]);       // 40072 / 40073
    telemetryOut.gridPointPowerW = parseInt32BE(&payload[12]);  // 40074 / 40075
    telemetryOut.batterySocPercent = parseUInt16BE(&payload[30]); // 40083
    telemetryOut.emsStatus = parseUInt16BE(&payload[34]);         // 40085

    return true;
}

bool E3dcModbusClient::readHoldingRegisters
(
    const IPAddress &ip,
    const uint16_t port,
    const uint8_t unitId,
    const uint16_t startRegister,
    const uint16_t count,
    uint8_t *buffer,
    const size_t bufferLen
)
{
    if ((count * 2U) > bufferLen)
    {
        logger_.error("modbus", "Target buffer too small");
        return false;
    }

    NetworkClient client;
    client.setTimeout(1500);

    if (!client.connect(ip, port))
    {
        logger_.warn("modbus", "Connection to E3/DC failed: " + ip.toString());
        return false;
    }

    ++transactionId_;

    uint8_t request[12]{};
    request[0] = static_cast<uint8_t>(transactionId_ >> 8);
    request[1] = static_cast<uint8_t>(transactionId_ & 0xFF);
    request[2] = 0x00; // Protocol ID high
    request[3] = 0x00; // Protocol ID low
    request[4] = 0x00; // Length high
    request[5] = 0x06; // Length low
    request[6] = unitId;
    request[7] = 0x03; // Read holding registers
    request[8] = static_cast<uint8_t>(startRegister >> 8);
    request[9] = static_cast<uint8_t>(startRegister & 0xFF);
    request[10] = static_cast<uint8_t>(count >> 8);
    request[11] = static_cast<uint8_t>(count & 0xFF);

    if (client.write(request, sizeof(request)) != sizeof(request))
    {
        logger_.warn("modbus", "Write request failed");
        client.stop();
        return false;
    }

    uint8_t responseHeader[9]{};
    constexpr size_t expectedHeader = sizeof(responseHeader);

    if (client.readBytes(responseHeader, expectedHeader) != expectedHeader)
    {
        logger_.warn("modbus", "Short Modbus response header");
        client.stop();
        return false;
    }

    const uint16_t responseTransactionId =
        (static_cast<uint16_t>(responseHeader[0]) << 8U) |
        static_cast<uint16_t>(responseHeader[1]);

    if (responseTransactionId != transactionId_)
    {
        logger_.warn("modbus", "Unexpected transaction ID");
        client.stop();
        return false;
    }

    const uint8_t responseUnitId = responseHeader[6];
    const uint8_t functionCode = responseHeader[7];
    const uint8_t byteCount = responseHeader[8];

    if (responseUnitId != unitId)
    {
        logger_.warn("modbus", "Unexpected unit ID: " + String(responseUnitId));
        client.stop();
        return false;
    }

    if (functionCode != 0x03)
    {
        logger_.warn("modbus", "Unexpected function code: " + String(functionCode));
        client.stop();
        return false;
    }

    if (byteCount != count * 2U)
    {
        logger_.warn("modbus", "Unexpected byte count: " + String(byteCount));
        client.stop();
        return false;
    }

    if (client.readBytes(buffer, byteCount) != byteCount)
    {
        logger_.warn("modbus", "Short Modbus payload");
        client.stop();
        return false;
    }

    client.stop();
    return true;
}

int32_t E3dcModbusClient::parseInt32BE(const uint8_t *data)
{
    const uint32_t raw =
        (static_cast<uint32_t>(data[2]) << 24U) |
        (static_cast<uint32_t>(data[3]) << 16U) |
        (static_cast<uint32_t>(data[0]) << 8U)  |
        static_cast<uint32_t>(data[1]);

    return static_cast<int32_t>(raw);
}

uint16_t E3dcModbusClient::parseUInt16BE(const uint8_t *data)
{
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[0]) << 8U) |
        static_cast<uint16_t>(data[1]));
}