#pragma once

#include <Arduino.h>

class UltrasonicSensor
{
public:
    UltrasonicSensor(int trigPin, int echoPin);

    void begin() const;
    bool readDistanceMm(float &distanceMm) const;

private:
    int trigPin_;
    int echoPin_;
};
