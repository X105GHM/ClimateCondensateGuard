#include "EthernetManager.hpp"

#include <Arduino.h>
#include <ETH.h>
#include <Network.h>
#include <SPI.h>

#include "config/AppConfig.hpp"

namespace
{
    constexpr eth_phy_type_t kEthType = ETH_PHY_W5500;
    constexpr int32_t kEthAddr = 1;
}

EthernetManager *EthernetManager::instance_ = nullptr;

EthernetManager::EthernetManager(Logger &logger, SharedState &sharedState)
    : logger_(logger), sharedState_(sharedState)
{
    instance_ = this;
}

bool EthernetManager::begin()
{
    SPI.begin(
        config::Pins::kEthSclk,
        config::Pins::kEthMiso,
        config::Pins::kEthMosi,
        config::Pins::kEthCs);

    Network.onEvent(EthernetManager::onNetworkEvent);

    const bool ok = ETH.begin
    (
        kEthType,
        kEthAddr,
        config::Pins::kEthCs,
        config::Pins::kEthInt,
        config::Pins::kEthRst,
        SPI
    );

    if (ok)
    {
        logger_.info("eth", "ETH.begin() started");
    }
    else
    {
        logger_.error("eth", "ETH.begin() failed");
    }

    return ok;
}

bool EthernetManager::connected() const
{
    return connected_;
}

String EthernetManager::ipAddressString() const
{
    return ETH.localIP().toString();
}

void EthernetManager::handleEvent(const arduino_event_id_t eventId)
{
    switch (eventId)
    {
    case ARDUINO_EVENT_ETH_START:
        ETH.setHostname("condensate-controller");
        logger_.info("eth", "Ethernet started");
        break;

    case ARDUINO_EVENT_ETH_CONNECTED:
        logger_.info("eth", "Ethernet link connected");
        break;

    case ARDUINO_EVENT_ETH_GOT_IP:
        connected_ = true;
        logger_.info("eth", "IP: " + ETH.localIP().toString());
        sharedState_.update([&](SystemStateData &state)
                            {
            state.ethernetConnected = true;
            state.ethernetIp = ETH.localIP().toString(); });
        break;

#ifdef ARDUINO_EVENT_ETH_LOST_IP
    case ARDUINO_EVENT_ETH_LOST_IP:
        connected_ = false;
        logger_.warn("eth", "Ethernet lost IP");
        sharedState_.update([&](SystemStateData &state)
        {
            state.ethernetConnected = false;
            state.ethernetIp = ""; 
        });
        break;
#endif

    case ARDUINO_EVENT_ETH_DISCONNECTED:
        connected_ = false;
        logger_.warn("eth", "Ethernet disconnected");
        sharedState_.update([&](SystemStateData &state)
                            {
            state.ethernetConnected = false;
            state.ethernetIp = ""; });
        break;

    case ARDUINO_EVENT_ETH_STOP:
        connected_ = false;
        logger_.warn("eth", "Ethernet stopped");
        sharedState_.update([&](SystemStateData &state)
                            {
            state.ethernetConnected = false;
            state.ethernetIp = ""; });
        break;

    default:
        break;
    }
}

void EthernetManager::onNetworkEvent(arduino_event_id_t eventId, arduino_event_info_t info)
{
    (void)info;
    if (instance_ != nullptr)
    {
        instance_->handleEvent(eventId);
    }
}