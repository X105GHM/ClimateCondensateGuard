#pragma once

#include <Arduino.h>

class HallCurrentSensor
{
public:
    HallCurrentSensor(int pin, float adcRefMv, float adcMaxCounts, float zeroCurrentMv, float sensitivityMvPerA);

    void begin() const;
    float readCurrentA(uint8_t samples = 8) const;
    void setZeroCurrentMv(float zeroCurrentMv);

private:
    int pin_;
    float adcRefMv_;
    float adcMaxCounts_;
    float zeroCurrentMv_;
    float sensitivityMvPerA_;
};
