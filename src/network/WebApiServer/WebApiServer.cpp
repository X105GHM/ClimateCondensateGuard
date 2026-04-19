#include "WebApiServer.hpp"

#include <Update.h>
#include <esp_system.h>

#include "config/AppConfig.hpp"
#include "app/ControlLogic.hpp"
#include "core/Types.hpp"

WebApiServer::WebApiServer(Logger &logger, SharedState &sharedState, ControlLogic &controlLogic)
    : logger_(logger),
      sharedState_(sharedState),
      controlLogic_(controlLogic),
      server_(80)
{
}

void WebApiServer::begin()
{
    generateOtaCsrfToken_();
    registerRoutes();
    server_.begin();
    logger_.info("http", "Web server started on port 80");
}

void WebApiServer::update()
{
    server_.handleClient();
}

void WebApiServer::registerRoutes()
{
    server_.on("/", HTTP_GET, [this]()
    {
        handleRoot();
    });

    server_.on("/api/status", HTTP_GET, [this]()
    {
        handleStatus();
    });

    server_.on("/api/logs", HTTP_GET, [this]()
    {
        handleLogs();
    });

    server_.on("/api/mode", HTTP_POST, [this]()
    {
        handleMode();
    });

    server_.on("/api/reset-faults", HTTP_POST, [this]()
    {
        handleResetFaults();
    });

    server_.on("/api/pump/prime", HTTP_POST, [this]()
    {
        handlePumpPrime();
    });

    server_.on("/ota", HTTP_GET, [this]()
    {
        handleOtaPage();
    });

    server_.on(
        "/api/ota",
        HTTP_POST,
        [this]()
        {
            handleOtaFinish();
        },
        [this]()
        {
            handleOtaUpload();
        });

    server_.onNotFound([this]()
    {
        server_.send(404, "application/json", "{\"error\":\"not_found\"}");
    });
}

void WebApiServer::handleRoot()
{
    const auto snapshot = sharedState_.snapshot();

    String html;
    html.reserve(5000);

    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Condensate Controller</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;max-width:900px;margin:2rem auto;padding:0 1rem;}";
    html += "button{margin:.25rem;padding:.6rem 1rem;}";
    html += "pre{background:#111;color:#ddd;padding:1rem;border-radius:8px;overflow:auto;}";
    html += ".card{border:1px solid #ccc;border-radius:10px;padding:1rem;margin-bottom:1rem;}";
    html += "a.button{display:inline-block;margin:.25rem;padding:.6rem 1rem;border:1px solid #888;border-radius:6px;text-decoration:none;color:#000;background:#eee;}";
    html += "</style>";
    html += "</head><body>";

    html += "<h1>ESP32 Klima / Kondensatpumpe</h1>";

    html += "<div class='card'>";
    html += "<strong>IP:</strong> " + htmlEscape(snapshot.ethernetIp) + "<br>";
    html += "<strong>Mode:</strong> " + htmlEscape(toString(snapshot.mode)) + "<br>";
    html += "<strong>Pumpe:</strong> " + String(snapshot.actuators.pumpEnabled ? "AN" : "AUS") + "<br>";
    html += "<strong>Klima:</strong> " + String(snapshot.actuators.climateEnabled ? "AN" : "AUS") + "<br>";
    html += "<strong>Climate Net:</strong> " + String(snapshot.climateNetwork.enabled ? "enabled" : "disabled") + "<br>";
    html += "<strong>Climate Net Online:</strong> " + String(snapshot.climateNetwork.online ? "YES" : "NO") + "<br>";
    html += "<strong>Climate Net Error:</strong> " + htmlEscape(snapshot.climateNetwork.lastError) + "<br>";
    html += "<strong>Buzzer:</strong> " + String(snapshot.actuators.buzzerEnabled ? "AN" : "AUS") + "<br>";
    html += "<strong>Level:</strong> " + String(snapshot.sensors.levelMm, 1) + " mm<br>";
    html += "<strong>Distanz:</strong> " + String(snapshot.sensors.distanceMm, 1) + " mm<br>";
    html += "<strong>Overflow Sensor:</strong> " + String(snapshot.sensors.overflowSensorWet ? "WET" : "dry") + "<br>";
    html += "<strong>Pumpenstrom:</strong> " + String(snapshot.sensors.pumpCurrentA, 2) + " A<br>";
    html += "<strong>PV:</strong> " + String(snapshot.energy.pvPowerW) + " W<br>";
    html += "<strong>Haus:</strong> " + String(snapshot.energy.housePowerW) + " W<br>";
    html += "<strong>Netzpunkt:</strong> " + String(snapshot.energy.gridPointPowerW) + " W<br>";
    html += "<strong>Battery SOC:</strong> " + String(snapshot.energy.batterySocPercent) + " %<br>";
    html += "<strong>Fault Mask:</strong> 0x" + String(snapshot.faultMask, HEX);
    html += "</div>";

    html += "<div class='card'>";
    html += "<form method='post' action='/api/mode?value=auto'><button>Auto</button></form>";
    html += "<form method='post' action='/api/mode?value=forced_on'><button>Forced ON</button></form>";
    html += "<form method='post' action='/api/mode?value=forced_off'><button>Forced OFF</button></form>";
    html += "<form method='post' action='/api/reset-faults'><button>Faults reset</button></form>";
    html += "<form method='post' action='/api/pump/prime?seconds=10'><button>Pumpe 10s primen</button></form>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<p>";
    html += "<a href='/api/status'>/api/status</a> | ";
    html += "<a href='/api/logs'>/api/logs</a> | ";
    html += "<a class='button' href='/ota'>OTA Firmware Update</a>";
    html += "</p>";
    html += "</div>";

    html += "</body></html>";

    server_.send(200, "text/html; charset=utf-8", html);
}

void WebApiServer::handleStatus()
{
    sendJsonStatus();
}

void WebApiServer::handleLogs()
{
    const int limit = server_.hasArg("limit") ? server_.arg("limit").toInt() : 100;
    const int safeLimit = constrain(limit, 1, 300);
    server_.send(200, "application/json", logger_.latestAsJson(static_cast<size_t>(safeLimit)));
}

void WebApiServer::handleMode()
{
    if (!server_.hasArg("value"))
    {
        server_.send(400, "application/json", "{\"error\":\"missing value\"}");
        return;
    }

    const String value = server_.arg("value");

    if (value == "auto")
    {
        controlLogic_.requestMode(OperationMode::Auto);
    }
    else if (value == "forced_on")
    {
        controlLogic_.requestMode(OperationMode::ForcedOn);
    }
    else if (value == "forced_off")
    {
        controlLogic_.requestMode(OperationMode::ForcedOff);
    }
    else
    {
        server_.send(400, "application/json", "{\"error\":\"invalid mode\"}");
        return;
    }

    logger_.info("http", "Mode changed via HTTP to " + value);
    sendJsonStatus();
}

void WebApiServer::handleResetFaults()
{
    controlLogic_.requestFaultReset();
    logger_.warn("http", "Fault reset requested via HTTP");
    sendJsonStatus();
}

void WebApiServer::handlePumpPrime()
{
    const int seconds = server_.hasArg("seconds") ? server_.arg("seconds").toInt() : 5;
    const uint32_t clampedSeconds = static_cast<uint32_t>(constrain(seconds, 1, 60));

    controlLogic_.requestPumpPrime(clampedSeconds * 1000UL);
    logger_.info("http", "Pump prime requested: " + String(clampedSeconds) + "s");

    sendJsonStatus();
}

void WebApiServer::handleOtaPage()
{
    if (!checkOtaAuth_(true))
    {
        return;
    }

    if (!config::kOta.enabled)
    {
        server_.send(403, "text/plain", "OTA disabled");
        return;
    }

    String html;
    html.reserve(3000);

    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>OTA Firmware Update</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;max-width:760px;margin:2rem auto;padding:0 1rem;}";
    html += ".card{border:1px solid #ccc;border-radius:10px;padding:1rem;margin-bottom:1rem;}";
    html += "button{padding:.7rem 1.1rem;margin-top:1rem;}";
    html += "code{background:#eee;padding:.1rem .3rem;border-radius:4px;}";
    html += ".warn{color:#a00000;font-weight:bold;}";
    html += "</style></head><body>";

    html += "<h1>OTA Firmware Update</h1>";

    html += "<div class='card'>";
    html += "<p class='warn'>Nur eine passende firmware.bin hochladen.</p>";
    html += "<p>PlatformIO-BIN liegt nach dem Build normalerweise hier:</p>";
    html += "<p><code>.pio/build/waveshare_esp32_s3_eth/firmware.bin</code></p>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<form method='POST' action='/api/ota?csrf=" + htmlEscape(otaCsrfToken_) + "' enctype='multipart/form-data'>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<br>";
    html += "<button type='submit'>Firmware hochladen und ESP neu starten</button>";
    html += "</form>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<p><a href='/'>Zurück</a></p>";
    html += "</div>";

    html += "</body></html>";

    server_.send(200, "text/html; charset=utf-8", html);
}

void WebApiServer::handleOtaUpload()
{
    HTTPUpload &upload = server_.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        otaUploadRejected_ = false;
        otaSuccess_ = false;
        otaStatusMessage_ = "";
        otaBytesWritten_ = 0;
        otaStartMs_ = millis();

        if (!config::kOta.enabled)
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "OTA disabled";
            return;
        }

        if (!checkOtaAuth_(false))
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "OTA authentication failed";
            return;
        }

        if (!checkOtaCsrf_())
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "OTA CSRF token invalid";
            return;
        }

        logger_.warn("ota", "Firmware upload started: " + upload.filename);

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "Update.begin failed: " + String(Update.errorString());
            logger_.error("ota", otaStatusMessage_);
            return;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (otaUploadRejected_)
        {
            return;
        }

        if (upload.currentSize == 0)
        {
            return;
        }

        if (otaBytesWritten_ == 0 && upload.buf[0] != 0xE9)
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "Invalid firmware image: wrong magic byte";
            Update.abort();
            logger_.error("ota", otaStatusMessage_);
            return;
        }

        const size_t written = Update.write(upload.buf, upload.currentSize);

        if (written != upload.currentSize)
        {
            otaUploadRejected_ = true;
            otaStatusMessage_ = "Update.write failed: " + String(Update.errorString());
            Update.abort();
            logger_.error("ota", otaStatusMessage_);
            return;
        }

        otaBytesWritten_ += written;
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (otaUploadRejected_)
        {
            Update.abort();
            return;
        }

        if (Update.end(true))
        {
            otaSuccess_ = true;
            otaStatusMessage_ = "Update successful, bytes=" + String(otaBytesWritten_);
            logger_.warn("ota", otaStatusMessage_);
        }
        else
        {
            otaSuccess_ = false;
            otaUploadRejected_ = true;
            otaStatusMessage_ = "Update.end failed: " + String(Update.errorString());
            logger_.error("ota", otaStatusMessage_);
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        otaSuccess_ = false;
        otaUploadRejected_ = true;
        otaStatusMessage_ = "Upload aborted";
        Update.abort();
        logger_.error("ota", otaStatusMessage_);
    }
}

void WebApiServer::handleOtaFinish()
{
    if (!checkOtaAuth_(true))
    {
        return;
    }

    if (!config::kOta.enabled)
    {
        server_.send(403, "text/plain", "OTA disabled");
        return;
    }

    if (!checkOtaCsrf_())
    {
        server_.send(403, "text/plain", "Invalid CSRF token");
        return;
    }

    server_.sendHeader("Connection", "close");

    if (otaSuccess_ && !Update.hasError())
    {
        String html;
        html.reserve(1000);

        html += "<!doctype html><html><head><meta charset='utf-8'>";
        html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<meta http-equiv='refresh' content='8;url=/'>";
        html += "<title>OTA success</title>";
        html += "</head><body>";
        html += "<h1>Firmware update successful</h1>";
        html += "<p>Bytes written: " + String(otaBytesWritten_) + "</p>";
        html += "<p>ESP32 restarts now. Reload the page after a few seconds.</p>";
        html += "</body></html>";

        server_.send(200, "text/html; charset=utf-8", html);

        delay(800);
        ESP.restart();
        return;
    }

    String message = otaStatusMessage_;
    if (message.isEmpty())
    {
        message = "OTA failed";
    }

    server_.send(500, "text/plain", message);
}

void WebApiServer::sendJsonStatus()
{
    const auto snapshot = sharedState_.snapshot();

    String json;
    json.reserve(1400);

    json += "{";

    json += "\"ethernet\":{";
    json += "\"connected\":" + String(snapshot.ethernetConnected ? "true" : "false") + ",";
    json += "\"ip\":\"" + snapshot.ethernetIp + "\"},";

    json += "\"mode\":\"" + toString(snapshot.mode) + "\",";
    json += "\"fault_mask\":" + String(snapshot.faultMask) + ",";

    json += "\"sensors\":{";
    json += "\"ultrasonic_valid\":" + String(snapshot.sensors.ultrasonicValid ? "true" : "false") + ",";
    json += "\"distance_mm\":" + String(snapshot.sensors.distanceMm, 1) + ",";
    json += "\"level_mm\":" + String(snapshot.sensors.levelMm, 1) + ",";
    json += "\"overflow_wet\":" + String(snapshot.sensors.overflowSensorWet ? "true" : "false") + ",";
    json += "\"pump_current_a\":" + String(snapshot.sensors.pumpCurrentA, 2) + ",";
    json += "\"pump_current_detected\":" + String(snapshot.sensors.pumpCurrentDetected ? "true" : "false");
    json += "},";

    json += "\"energy\":{";
    json += "\"valid\":" + String(snapshot.energy.valid ? "true" : "false") + ",";
    json += "\"pv_w\":" + String(snapshot.energy.pvPowerW) + ",";
    json += "\"battery_w\":" + String(snapshot.energy.batteryPowerW) + ",";
    json += "\"house_w\":" + String(snapshot.energy.housePowerW) + ",";
    json += "\"grid_w\":" + String(snapshot.energy.gridPointPowerW) + ",";
    json += "\"battery_soc_percent\":" + String(snapshot.energy.batterySocPercent) + ",";
    json += "\"ems_status\":" + String(snapshot.energy.emsStatus) + ",";
    json += "\"cooling_allowed_auto\":" + String(snapshot.energy.coolingAllowedAuto ? "true" : "false");
    json += "},";

    json += "\"climate_network\":{";
    json += "\"enabled\":" + String(snapshot.climateNetwork.enabled ? "true" : "false") + ",";
    json += "\"online\":" + String(snapshot.climateNetwork.online ? "true" : "false") + ",";
    json += "\"last_command_ms\":" + String(snapshot.climateNetwork.lastCommandMs);
    json += "},";

    json += "\"actuators\":{";
    json += "\"pump_enabled\":" + String(snapshot.actuators.pumpEnabled ? "true" : "false") + ",";
    json += "\"climate_enabled\":" + String(snapshot.actuators.climateEnabled ? "true" : "false") + ",";
    json += "\"buzzer_enabled\":" + String(snapshot.actuators.buzzerEnabled ? "true" : "false");
    json += "},";

    json += "\"ota\":{";
    json += "\"enabled\":" + String(config::kOta.enabled ? "true" : "false") + ",";
    json += "\"last_success\":" + String(otaSuccess_ ? "true" : "false") + ",";
    json += "\"last_bytes\":" + String(otaBytesWritten_) + ",";
    json += "\"last_message\":\"" + htmlEscape(otaStatusMessage_) + "\"";
    json += "}";

    json += "}";

    server_.send(200, "application/json", json);
}

bool WebApiServer::checkOtaAuth_(const bool sendChallenge)
{
    if (!config::kOta.enabled)
    {
        return false;
    }

    if (config::kOta.username.isEmpty() && config::kOta.password.isEmpty())
    {
        return true;
    }

    const bool ok = server_.authenticate(
        config::kOta.username.c_str(),
        config::kOta.password.c_str());

    if (!ok && sendChallenge)
    {
        server_.requestAuthentication(BASIC_AUTH, "CondensateGuard OTA");
    }

    return ok;
}

bool WebApiServer::checkOtaCsrf_() const
{
    if (otaCsrfToken_.isEmpty())
    {
        return false;
    }

    if (!server_.hasArg("csrf"))
    {
        return false;
    }

    return server_.arg("csrf") == otaCsrfToken_;
}

void WebApiServer::generateOtaCsrfToken_()
{
    otaCsrfToken_ = String(static_cast<uint32_t>(esp_random()), HEX);
    otaCsrfToken_ += String(static_cast<uint32_t>(esp_random()), HEX);
    otaCsrfToken_ += String(static_cast<uint32_t>(millis()), HEX);
}

String WebApiServer::htmlEscape(const String &input)
{
    String out = input;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\"", "&quot;");
    out.replace("'", "&#39;");
    return out;
}