#pragma once

#include <Arduino.h>

class UltrasonicSensor
{
public:
    UltrasonicSensor(int trigPin, int echoPin);

    void begin();
    bool readDistanceMm(float &distanceMm);
    void resetFilter();

private:
    bool readRawDistanceMm_(float &distanceMm) const;
    bool isDistancePlausible_(float distanceMm) const;
    float applyFilter_(float rawDistanceMm);

    int trigPin_;
    int echoPin_;

    bool filterInitialized_ {false};
    float filteredDistanceMm_ {0.0F};

    uint8_t invalidReadCount_ {0};
};
