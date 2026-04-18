#include "WaterSensor.hpp"

WaterSensor::WaterSensor(const int pin, const bool activeHigh) : pin_(pin), activeHigh_(activeHigh){}

void WaterSensor::begin() const
{
    pinMode(pin_, INPUT);
}

bool WaterSensor::isWet() const
{
    const bool raw = digitalRead(pin_) == HIGH;
    return activeHigh_ ? raw : !raw;
}
