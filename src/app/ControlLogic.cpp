#include "ControlLogic.hpp"

#include "config/AppConfig.hpp"

namespace
{
    constexpr uint32_t kFaultMaskPumpIssues = static_cast<uint32_t>(kFaultPumpNoCurrent) | static_cast<uint32_t>(kFaultPumpNoDrain);

    bool isPumpFault(const uint32_t mask)
    {
        return (mask & kFaultMaskPumpIssues) != 0U;
    }

    bool timeBefore(const uint32_t now, const uint32_t target)
    {
        return static_cast<int32_t>(now - target) < 0;
    }
}

ControlLogic::ControlLogic
(
    Logger &logger,
    SharedState &sharedState,
    PumpController &pump,
    ClimateController &climate,
    BuzzerController &buzzer
)
    : logger_(logger),
      sharedState_(sharedState),
      pump_(pump),
      climate_(climate),
      buzzer_(buzzer)
{}

void ControlLogic::tick()
{
    const uint32_t now = millis();
    SystemStateData snapshot = sharedState_.snapshot();

    uint32_t liveFaults = 0U;

    if (resetRequested_.exchange(false))
    {
        if (snapshot.sensors.overflowSensorWet)
        {
            logger_.warn("ctrl", "Fault reset ignored because overflow sensor is still wet");
        }
        else
        {
            latchedFaults_ = 0U;

            pumpCommandSinceMs_ = 0U;
            pumpStartLevelMm_ = snapshot.sensors.levelMm;
            forcedPumpUntilMs_ = 0U;
            lastPumpCommand_ = false;

            faultResetGraceUntilMs_ = now + 2000U;

            buzzer_.setAlarm(false);

            sharedState_.update([&](SystemStateData &state)
            {
                state.faultMask = 0U;
                state.actuators.buzzerEnabled = false;
            });

            logger_.warn("ctrl", "Fault latch cleared by user request");
        }
    }

    const bool faultResetGraceActive = faultResetGraceUntilMs_ != 0U && timeBefore(now, faultResetGraceUntilMs_);

    const OperationMode requestedMode = requestedMode_.load();

    if (snapshot.mode != requestedMode)
    {
        logger_.info("ctrl", "Mode changed to " + toString(requestedMode));
    }

    const uint32_t primeMs = primeDurationMs_.exchange(0U);
    if (primeMs > 0U)
    {
        forcedPumpUntilMs_ = now + primeMs;
        logger_.info("ctrl", "Pump prime active for " + String(primeMs) + " ms");
    }

    setFault(liveFaults, kFaultOverflowSensor, snapshot.sensors.overflowSensorWet);
    setFault(liveFaults, kFaultUltrasonicInvalid, !snapshot.sensors.ultrasonicValid);
    setFault(liveFaults, kFaultEnergyOffline, config::kEnergy.enabled && !snapshot.energy.valid);

    const bool highLevel = snapshot.sensors.ultrasonicValid && (snapshot.sensors.levelMm >= config::kTank.pumpOnLevelMm);

    const bool lowLevel = snapshot.sensors.ultrasonicValid && (snapshot.sensors.levelMm <= config::kTank.pumpOffLevelMm);

    const bool overflow = snapshot.sensors.overflowSensorWet;
    const bool primeActive = now < forcedPumpUntilMs_;

    const bool pumpNoCurrentLatched = hasFault(latchedFaults_, kFaultPumpNoCurrent);
    const bool pumpNoDrainLatched = hasFault(latchedFaults_, kFaultPumpNoDrain);

    bool desiredPump = false;

    if (pumpNoCurrentLatched)
    {
        desiredPump = false;
    }
    else if (pumpNoDrainLatched && !overflow)
    {
        desiredPump = false;
    }
    else if (overflow || primeActive)
    {
        desiredPump = true;
    }
    else if (lastPumpCommand_)
    {
        desiredPump = !lowLevel;
    }
    else
    {
        desiredPump = highLevel;
    }

    const bool pumpStartsNow = desiredPump && (!lastPumpCommand_ || pumpCommandSinceMs_ == 0U);

    if (pumpStartsNow)
    {
        pumpCommandSinceMs_ = now;
        pumpStartLevelMm_ = snapshot.sensors.levelMm;
        logger_.info("ctrl", "Pump start requested");
    }

    if (desiredPump)
    {
        const uint32_t runtimeMs = now - pumpCommandSinceMs_;

        if (!faultResetGraceActive &&
            runtimeMs >= config::kPumpSafety.noCurrentFaultDelayMs &&
            !snapshot.sensors.pumpCurrentDetected)
        {
            if (!hasFault(latchedFaults_, kFaultPumpNoCurrent))
            {
                logger_.error("ctrl", "Pump commanded but no current detected");
            }

            setFault(latchedFaults_, kFaultPumpNoCurrent, true);
        }

        if (!faultResetGraceActive &&
            runtimeMs >= config::kPumpSafety.noDrainFaultDelayMs &&
            snapshot.sensors.pumpCurrentDetected &&
            snapshot.sensors.ultrasonicValid)
        {
            const float dropMm = pumpStartLevelMm_ - snapshot.sensors.levelMm;

            if (dropMm < config::kPumpSafety.minRequiredLevelDropMm)
            {
                if (!hasFault(latchedFaults_, kFaultPumpNoDrain))
                {
                    logger_.error("ctrl", "Pump current flows but level does not fall");
                }

                setFault(latchedFaults_, kFaultPumpNoDrain, true);
            }
        }
    }

    const uint32_t effectiveFaults = liveFaults | latchedFaults_;

    if (hasFault(effectiveFaults, kFaultPumpNoCurrent))
    {
        desiredPump = false;
    }

    if (hasFault(effectiveFaults, kFaultPumpNoDrain) && !overflow)
    {
        desiredPump = false;
    }

    if (!desiredPump)
    {
        pumpCommandSinceMs_ = 0U;
        pumpStartLevelMm_ = snapshot.sensors.levelMm;
    }

    autoCoolingAllowed_ = evaluateAutomaticCoolingPermission(snapshot);

    bool climateWanted = false;

    switch (requestedMode)
    {
    case OperationMode::Auto:
        climateWanted = autoCoolingAllowed_;
        break;

    case OperationMode::ForcedOn:
        climateWanted = true;
        break;

    case OperationMode::ForcedOff:
        climateWanted = false;
        break;
    }

    const bool criticalFault =
        hasFault(effectiveFaults, kFaultOverflowSensor) ||
        isPumpFault(effectiveFaults);

    if (criticalFault)
    {
        climateWanted = false;
    }

    const bool buzzerWanted = criticalFault;

    pump_.setEnabled(desiredPump);
    climate_.setEnabled(climateWanted);
    buzzer_.setAlarm(buzzerWanted);

    lastPumpCommand_ = desiredPump;

    sharedState_.update([&](SystemStateData &state)
    {
        state.mode = requestedMode;
        state.faultMask = effectiveFaults;
        state.energy.coolingAllowedAuto = autoCoolingAllowed_;
        state.actuators.pumpEnabled = desiredPump;
        state.actuators.climateEnabled = climateWanted;
        state.actuators.buzzerEnabled = buzzerWanted;
    });
}

void ControlLogic::requestMode(const OperationMode mode)
{
    requestedMode_.store(mode);
}

void ControlLogic::requestFaultReset()
{
    resetRequested_.store(true);
}

void ControlLogic::requestPumpPrime(const uint32_t durationMs)
{
    primeDurationMs_.store(durationMs);
}

bool ControlLogic::evaluateAutomaticCoolingPermission(const SystemStateData &snapshot)
{
    if (!config::kEnergy.enabled)
    {
        return true;
    }

    if (!snapshot.energy.valid)
    {
        return false;
    }

    bool rawAllowed = false;

    if (autoCoolingAllowed_)
    {
        if (snapshot.energy.gridPointPowerW > config::kEnergy.stopImportThresholdW ||
            snapshot.energy.batterySocPercent < config::kEnergy.minBatterySocPercent)
        {
            rawAllowed = false;
        }
        else
        {
            rawAllowed = true;
        }
    }
    else if (snapshot.energy.gridPointPowerW <= -config::kEnergy.startExportThresholdW &&
             snapshot.energy.batterySocPercent >= config::kEnergy.minBatterySocPercent)
    {
        rawAllowed = true;
    }

    if (rawAllowed == autoCoolingAllowed_)
    {
        autoCoolingCandidateSinceMs_ = 0U;
        autoCoolingCandidateAllowed_ = rawAllowed;
        return autoCoolingAllowed_;
    }

    const uint32_t now = millis();

    if (autoCoolingCandidateSinceMs_ == 0U || autoCoolingCandidateAllowed_ != rawAllowed)
    {
        autoCoolingCandidateAllowed_ = rawAllowed;
        autoCoolingCandidateSinceMs_ = now;
        return autoCoolingAllowed_;
    }

    if ((now - autoCoolingCandidateSinceMs_) < config::kEnergy.decisionHoldMs)
    {
        return autoCoolingAllowed_;
    }

    autoCoolingCandidateSinceMs_ = 0U;
    autoCoolingCandidateAllowed_ = rawAllowed;

    if (rawAllowed)
    {
        logger_.info("energy", "Cooling auto permit granted (stable)");
    }
    else
    {
        logger_.warn("energy", "Cooling auto permit removed (stable)");
    }

    return rawAllowed;
}

String ControlLogic::faultsToText(const uint32_t faultMask) const
{
    String text;

    if (hasFault(faultMask, kFaultOverflowSensor))
    {
        text += "overflow_sensor;";
    }

    if (hasFault(faultMask, kFaultPumpNoCurrent))
    {
        text += "pump_no_current;";
    }

    if (hasFault(faultMask, kFaultPumpNoDrain))
    {
        text += "pump_no_drain;";
    }

    if (hasFault(faultMask, kFaultUltrasonicInvalid))
    {
        text += "ultrasonic_invalid;";
    }

    if (hasFault(faultMask, kFaultEnergyOffline))
    {
        text += "energy_offline;";
    }

    return text;
}