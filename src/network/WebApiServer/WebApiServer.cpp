#include "WebApiServer.hpp"

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
               { handleRoot(); });
    server_.on("/api/status", HTTP_GET, [this]()
               { handleStatus(); });
    server_.on("/api/logs", HTTP_GET, [this]()
               { handleLogs(); });
    server_.on("/api/mode", HTTP_POST, [this]()
               { handleMode(); });
    server_.on("/api/reset-faults", HTTP_POST, [this]()
               { handleResetFaults(); });
    server_.on("/api/pump/prime", HTTP_POST, [this]()
               { handlePumpPrime(); });

    server_.onNotFound([this]()
                       { server_.send(404, "application/json", "{\"error\":\"not_found\"}"); });
}

void WebApiServer::handleRoot()
{
    const auto snapshot = sharedState_.snapshot();

    String html;
    html.reserve(4096);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Condensate Controller</title>";
    html += "<style>body{font-family:Arial,sans-serif;max-width:900px;margin:2rem auto;padding:0 1rem;}button{margin:.25rem;padding:.6rem 1rem;}pre{background:#111;color:#ddd;padding:1rem;border-radius:8px;overflow:auto;} .card{border:1px solid #ccc;border-radius:10px;padding:1rem;margin-bottom:1rem;}</style>";
    html += "</head><body>";
    html += "<h1>ESP32 Klima / Kondensatpumpe</h1>";
    html += "<div class='card'><strong>IP:</strong> " + htmlEscape(snapshot.ethernetIp) + "<br>";
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
    html += "<strong>Fault Mask:</strong> 0x" + String(snapshot.faultMask, HEX) + "</div>";

    html += "<div class='card'>";
    html += "<form method='post' action='/api/mode?value=auto'><button>Auto</button></form>";
    html += "<form method='post' action='/api/mode?value=forced_on'><button>Forced ON</button></form>";
    html += "<form method='post' action='/api/mode?value=forced_off'><button>Forced OFF</button></form>";
    html += "<form method='post' action='/api/reset-faults'><button>Faults reset</button></form>";
    html += "<form method='post' action='/api/pump/prime?seconds=10'><button>Pumpe 10s primen</button></form>";
    html += "</div>";

    html += "<div class='card'><p><a href='/api/status'>/api/status</a> | <a href='/api/logs'>/api/logs</a></p></div>";

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

void WebApiServer::sendJsonStatus()
{
    const auto snapshot = sharedState_.snapshot();

    String json;
    json.reserve(1024);
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
    json += "}";
    json += "}";

    server_.send(200, "application/json", json);
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
