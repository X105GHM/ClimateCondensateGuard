#pragma once

#include <Arduino.h>

class PumpController
{
public:
    explicit PumpController(int pin, bool activeHigh = true);

    void begin();
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;

private:
    int pin_;
    bool activeHigh_;
    bool enabled_{false};
};
