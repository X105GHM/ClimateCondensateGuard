#include "UltrasonicSensor.hpp"

UltrasonicSensor::UltrasonicSensor(const int trigPin, const int echoPin) : trigPin_(trigPin), echoPin_(echoPin)
{
}

void UltrasonicSensor::begin() const
{
    pinMode(trigPin_, OUTPUT);
    pinMode(echoPin_, INPUT);
    digitalWrite(trigPin_, LOW);
}

bool UltrasonicSensor::readDistanceMm(float &distanceMm) const
{
    digitalWrite(trigPin_, LOW);
    delayMicroseconds(3);
    digitalWrite(trigPin_, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin_, LOW);

    const unsigned long durationUs = pulseIn(echoPin_, HIGH, 30000UL);
    if (durationUs == 0UL)
    {
        return false;
    }

    distanceMm = static_cast<float>(durationUs) * 0.343F / 2.0F;
    return true;
}
