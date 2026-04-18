#pragma once

#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"

class EthernetManager
{
public:
    EthernetManager(Logger &logger, SharedState &sharedState);

    bool begin();
    [[nodiscard]] bool connected() const;
    [[nodiscard]] String ipAddressString() const;

private:
    void handleEvent(arduino_event_id_t eventId);
    static void onNetworkEvent(arduino_event_id_t eventId, arduino_event_info_t info);

    Logger &logger_;
    SharedState &sharedState_;
    bool connected_{false};

    static EthernetManager *instance_;
};