#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"

class ControlLogic;

class WebApiServer
{
public:
    WebApiServer(Logger &logger, SharedState &sharedState, ControlLogic &controlLogic);

    void begin();
    void update();

private:
    void registerRoutes();
    void handleRoot();
    void handleStatus();
    void handleLogs();
    void handleMode();
    void handleResetFaults();
    void handlePumpPrime();
    void sendJsonStatus();

    static String htmlEscape(const String &input);

    Logger &logger_;
    SharedState &sharedState_;
    ControlLogic &controlLogic_;
    WebServer server_;
};
