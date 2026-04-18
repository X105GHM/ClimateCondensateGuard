#include "PumpController.hpp"

PumpController::PumpController(const int pin, const bool activeHigh) : pin_(pin), activeHigh_(activeHigh){}

void PumpController::begin()
{
    pinMode(pin_, OUTPUT);
    setEnabled(false);
}

void PumpController::setEnabled(const bool enabled)
{
    enabled_ = enabled;
    const bool raw = activeHigh_ ? enabled_ : !enabled_;
    digitalWrite(pin_, raw ? HIGH : LOW);
}

bool PumpController::isEnabled() const
{
    return enabled_;
}
