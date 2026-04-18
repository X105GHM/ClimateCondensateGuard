#pragma once

#include <Arduino.h>

class WaterSensor
{
public:
    WaterSensor(int pin, bool activeHigh);

    void begin() const;
    [[nodiscard]] bool isWet() const;

private:
    int pin_;
    bool activeHigh_;
};
