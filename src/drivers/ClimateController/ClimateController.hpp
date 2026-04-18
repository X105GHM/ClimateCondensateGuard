#pragma once

#include <Arduino.h>

class Logger;
class SharedState;

class ClimateController
{
public:
    ClimateController(Logger &logger, SharedState &sharedState, int localPin = -1, bool activeHigh = true);

    void begin();

    void setEnabled(bool enabled);

    [[nodiscard]] bool isEnabled() const;

private:
    void setLocalOutput_(bool enabled);
    bool postJson_(const String &path, const String &jsonBody);
    void updateNetworkState_(bool ok, const String &message);

    Logger &logger_;
    SharedState &sharedState_;

    int localPin_{-1};
    bool activeHigh_{true};

    bool enabled_{false};
    bool firstCommand_{true};

    uint32_t lastNetSyncMs_{0};
};