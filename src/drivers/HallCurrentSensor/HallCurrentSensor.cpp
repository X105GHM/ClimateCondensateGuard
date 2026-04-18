#include "HallCurrentSensor.hpp"

HallCurrentSensor::HallCurrentSensor
(
    const int pin,
    const float adcRefMv,
    const float adcMaxCounts,
    const float zeroCurrentMv,
    const float sensitivityMvPerA
)
    : pin_(pin),
      adcRefMv_(adcRefMv),
      adcMaxCounts_(adcMaxCounts),
      zeroCurrentMv_(zeroCurrentMv),
      sensitivityMvPerA_(sensitivityMvPerA)
{}

void HallCurrentSensor::begin() const
{
    analogReadResolution(12);
    analogSetPinAttenuation(pin_, ADC_11db);
}

float HallCurrentSensor::readCurrentA(const uint8_t samples) const
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; ++i)
    {
        sum += analogRead(pin_);
        delay(2);
    }

    const float counts = static_cast<float>(sum) / static_cast<float>(samples);
    const float milliVolts = (counts / adcMaxCounts_) * adcRefMv_;
    const float amps = fabsf((milliVolts - zeroCurrentMv_) / sensitivityMvPerA_);
    return amps;
}

void HallCurrentSensor::setZeroCurrentMv(const float zeroCurrentMv)
{
    zeroCurrentMv_ = zeroCurrentMv;
}
