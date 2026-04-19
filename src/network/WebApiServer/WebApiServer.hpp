#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "app/ControlLogic.hpp"
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"

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

    void handleOtaPage();
    void handleOtaUpload();
    void handleOtaFinish();

    void sendJsonStatus();

    bool checkOtaAuth_(bool sendChallenge);
    bool checkOtaCsrf_() const;
    void generateOtaCsrfToken_();

    static String htmlEscape(const String &input);

    Logger &logger_;
    SharedState &sharedState_;
    ControlLogic &controlLogic_;

    WebServer server_;

    String otaCsrfToken_;
    bool otaUploadRejected_ {false};
    bool otaSuccess_ {false};
    String otaStatusMessage_;
    size_t otaBytesWritten_ {0};
    uint32_t otaStartMs_ {0};
};