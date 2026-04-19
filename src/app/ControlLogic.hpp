#pragma once

#include <Arduino.h>
#include <atomic>

#include "core/Types.hpp"
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"
#include "drivers/PumpController/PumpController.hpp"
#include "drivers/ClimateController/ClimateController.hpp"
#include "drivers/BuzzerController/BuzzerController.hpp"

class ControlLogic
{
public:
    ControlLogic(Logger &logger,
                 SharedState &sharedState,
                 PumpController &pump,
                 ClimateController &climate,
                 BuzzerController &buzzer);

    void tick();
    void requestMode(OperationMode mode);
    void requestFaultReset();
    void requestPumpPrime(uint32_t durationMs);

private:
    bool evaluateAutomaticCoolingPermission(const SystemStateData &snapshot);
    String faultsToText(uint32_t faultMask) const;

    Logger &logger_;
    SharedState &sharedState_;
    PumpController &pump_;
    ClimateController &climate_;
    BuzzerController &buzzer_;

    std::atomic<OperationMode> requestedMode_ {OperationMode::Auto};
    std::atomic<bool> resetRequested_ {false};
    std::atomic<uint32_t> primeDurationMs_ {0};

    uint32_t latchedFaults_ {0};

    bool autoCoolingAllowed_ {false};
    bool lastPumpCommand_ {false};

    uint32_t pumpCommandSinceMs_ {0};
    float pumpStartLevelMm_ {0.0F};

    uint32_t forcedPumpUntilMs_ {0};

    uint32_t faultResetGraceUntilMs_ {0};
};