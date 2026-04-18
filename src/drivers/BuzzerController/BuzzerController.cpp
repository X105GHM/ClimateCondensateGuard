#include "BuzzerController.hpp"

BuzzerController::BuzzerController(const int pin, const bool activeHigh) : pin_(pin), activeHigh_(activeHigh){}

void BuzzerController::begin()
{
    pinMode(pin_, OUTPUT);
    writeOutput(false);
}

void BuzzerController::setAlarm(const bool active)
{
    if (active == alarmActive_)
    {
        return;
    }

    alarmActive_ = active;
    phaseStartMs_ = millis();
    outputState_ = false;

    if (!alarmActive_)
    {
        writeOutput(false);
    }
}

void BuzzerController::update()
{
    if (!alarmActive_)
    {
        writeOutput(false);
        return;
    }

    const uint32_t now = millis();
    const uint32_t phaseTime = now - phaseStartMs_;
    const uint32_t maxPhase = outputState_ ? kOnMs : kOffMs;

    if (phaseTime >= maxPhase)
    {
        outputState_ = !outputState_;
        phaseStartMs_ = now;
        writeOutput(outputState_);
    }
    else
    {
        writeOutput(outputState_);
    }
}

bool BuzzerController::isAlarmActive() const
{
    return alarmActive_;
}

void BuzzerController::writeOutput(const bool on) const
{
    const bool raw = activeHigh_ ? on : !on;
    digitalWrite(pin_, raw ? HIGH : LOW);
}
