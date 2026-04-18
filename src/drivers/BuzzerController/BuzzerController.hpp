#pragma once

#include <Arduino.h>

class BuzzerController
{
public:
    explicit BuzzerController(int pin, bool activeHigh = true);

    void begin();
    void setAlarm(bool active);
    void update();
    [[nodiscard]] bool isAlarmActive() const;

private:
    void writeOutput(bool on) const;

    int pin_;
    bool activeHigh_;
    bool alarmActive_{false};
    bool outputState_{false};
    uint32_t phaseStartMs_{0};

    static constexpr uint32_t kOnMs = 180;
    static constexpr uint32_t kOffMs = 220;
};
