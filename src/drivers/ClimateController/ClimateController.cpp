#include "ClimateController.hpp"

#include <HTTPClient.h>
#include <NetworkClient.h>

#include "config/AppConfig.hpp"
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"

ClimateController::ClimateController
(
    Logger &logger,
    SharedState &sharedState,
    const int localPin,
    const bool activeHigh
)
    : logger_(logger),
      sharedState_(sharedState),
      localPin_(localPin),
      activeHigh_(activeHigh)
{}

void ClimateController::begin()
{
    if (config::kClimateNet.useLocalPin && localPin_ >= 0)
    {
        pinMode(localPin_, OUTPUT);
        setLocalOutput_(false);
    }

    sharedState_.update([&](SystemStateData &state)
    {
        state.climateNetwork.enabled = config::kClimateNet.enabled;
        state.climateNetwork.online = false;
        state.climateNetwork.lastCommandMs = 0;
        state.climateNetwork.lastError = ""; 
    });
}

void ClimateController::setLocalOutput_(const bool enabled)
{
    if (!config::kClimateNet.useLocalPin || localPin_ < 0)
    {
        return;
    }

    const bool raw = activeHigh_ ? enabled : !enabled;
    digitalWrite(localPin_, raw ? HIGH : LOW);
}

bool ClimateController::postJson_(const String &path, const String &jsonBody)
{
    if (config::kClimateNet.baseUrl.isEmpty())
    {
        updateNetworkState_(false, "bridge_base_url_empty");
        return false;
    }

    NetworkClient client;
    HTTPClient http;

    const String url = config::kClimateNet.baseUrl + path;

    if (!http.begin(client, url))
    {
        updateNetworkState_(false, "http_begin_failed");
        logger_.error("climate-net", "HTTP begin failed for " + url);
        return false;
    }

    http.setConnectTimeout(config::kClimateNet.timeoutMs);
    http.setTimeout(config::kClimateNet.timeoutMs);
    http.addHeader("Content-Type", "application/json");

    if (!config::kClimateNet.bearerToken.isEmpty())
    {
        http.addHeader("Authorization", "Bearer " + config::kClimateNet.bearerToken);
    }

    const int statusCode = http.POST(jsonBody);

    String responseBody;
    if (statusCode > 0)
    {
        responseBody = http.getString();
    }

    http.end();

    const bool ok = statusCode >= 200 && statusCode < 300;

    if (ok)
    {
        updateNetworkState_(true, "");
        logger_.info("climate-net", "POST " + path + " ok");
    }
    else
    {
        const String err =
            "POST " + path +
            " failed, code=" + String(statusCode) +
            ", body=" + responseBody;

        updateNetworkState_(false, err);
        logger_.error("climate-net", err);
    }

    return ok;
}

void ClimateController::updateNetworkState_(const bool ok, const String &message)
{
    sharedState_.update([&](SystemStateData &state)
    {
        state.climateNetwork.enabled = config::kClimateNet.enabled;
        state.climateNetwork.online = ok;
        state.climateNetwork.lastCommandMs = millis();
        state.climateNetwork.lastError = message; 
    });
}

void ClimateController::setEnabled(const bool enabled)
{
    const uint32_t now = millis();

    const bool stateChanged = enabled_ != enabled;
    const bool resyncDue = now - lastNetSyncMs_ >= config::kClimateNet.resyncMs;

    enabled_ = enabled;

    setLocalOutput_(enabled_);

    if (!config::kClimateNet.enabled)
    {
        updateNetworkState_(false, "climate_network_disabled");
        return;
    }

    if (!firstCommand_ && !stateChanged && !resyncDue)
    {
        return;
    }

    bool ok = false;

    if (enabled_)
    {
        const String configJson =
            "{\"mode\":\"" + config::kClimateNet.mode +
            "\",\"target_c\":" + String(config::kClimateNet.targetTempC) +
            ",\"fan\":\"" + config::kClimateNet.fan + "\"}";

        const bool configOk = postJson_("/api/climate/config", configJson);
        const bool powerOk = postJson_("/api/climate/power", "{\"on\":true}");

        ok = configOk && powerOk;

        logger_.info("climate", "Climate ON requested via network");
    }
    else
    {
        ok = postJson_("/api/climate/power", "{\"on\":false}");

        logger_.info("climate", "Climate OFF requested via network");
    }

    if (ok)
    {
        lastNetSyncMs_ = now;
        firstCommand_ = false;
    }
}

bool ClimateController::isEnabled() const
{
    return enabled_;
}