#include "UltrasonicSensor.hpp"

#include <math.h>

#include "config/AppConfig.hpp"

UltrasonicSensor::UltrasonicSensor(const int trigPin, const int echoPin) : trigPin_(trigPin), echoPin_(echoPin){}

void UltrasonicSensor::begin()
{
    pinMode(trigPin_, OUTPUT);
    pinMode(echoPin_, INPUT);

    digitalWrite(trigPin_, LOW);

    resetFilter();
}

void UltrasonicSensor::resetFilter()
{
    filterInitialized_ = false;
    filteredDistanceMm_ = 0.0F;
    invalidReadCount_ = 0;
}

bool UltrasonicSensor::readDistanceMm(float &distanceMm)
{
    float rawDistanceMm = 0.0F;

    const bool rawOk = readRawDistanceMm_(rawDistanceMm) && isDistancePlausible_(rawDistanceMm);

    if (!rawOk)
    {
        if (filterInitialized_ && invalidReadCount_ < config::kUltrasonic.maxInvalidHoldCount)
        {
            ++invalidReadCount_;
            distanceMm = filteredDistanceMm_;
            return true;
        }

        distanceMm = 0.0F;
        return false;
    }

    invalidReadCount_ = 0;

    distanceMm = applyFilter_(rawDistanceMm);
    return true;
}

bool UltrasonicSensor::readRawDistanceMm_(float &distanceMm) const
{
    digitalWrite(trigPin_, LOW);
    delayMicroseconds(3);

    digitalWrite(trigPin_, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin_, LOW);

    const unsigned long durationUs = pulseIn
    (
        echoPin_,
        HIGH,
        config::kUltrasonic.echoTimeoutUs
    );

    if (durationUs == 0UL)
    {
        return false;
    }

    distanceMm = static_cast<float>(durationUs) * 0.343F * 0.5F;

    return true;
}

bool UltrasonicSensor::isDistancePlausible_(const float distanceMm) const
{
    if (isnan(distanceMm) || isinf(distanceMm))
    {
        return false;
    }

    if (distanceMm < config::kTank.validMinMm)
    {
        return false;
    }

    if (distanceMm > config::kTank.validMaxMm)
    {
        return false;
    }

    return true;
}

float UltrasonicSensor::applyFilter_(const float rawDistanceMm)
{
    if (!config::kUltrasonic.filterEnabled)
    {
        filteredDistanceMm_ = rawDistanceMm;
        filterInitialized_ = true;
        return rawDistanceMm;
    }

    if (!filterInitialized_)
    {
        filteredDistanceMm_ = rawDistanceMm;
        filterInitialized_ = true;
        return filteredDistanceMm_;
    }

    const float jumpMm = fabsf(rawDistanceMm - filteredDistanceMm_);

    if (jumpMm > config::kUltrasonic.maxAcceptedJumpMm)
    {
        return filteredDistanceMm_;
    }

    filteredDistanceMm_ = filteredDistanceMm_ + config::kUltrasonic.filterAlpha * (rawDistanceMm - filteredDistanceMm_);

    return filteredDistanceMm_;
}