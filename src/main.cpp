#include <Arduino.h>
#include <LittleFS.h>

#include "config/AppConfig.hpp"
#include "core/Logger/Logger.hpp"
#include "core/SharedState/SharedState.hpp"
#include "drivers/UltrasonicSensor/UltrasonicSensor.hpp"
#include "drivers/WaterSensor/WaterSensor.hpp"
#include "drivers/HallCurrentSensor/HallCurrentSensor.hpp"
#include "drivers/PumpController/PumpController.hpp"
#include "drivers/ClimateController/ClimateController.hpp"
#include "drivers/BuzzerController/BuzzerController.hpp"
#include "network/EthernetManager/EthernetManager.hpp"
#include "network/E3dcModbusClient/E3dcModbusClient.hpp"
#include "network/WebApiServer/WebApiServer.hpp"
#include "app/ControlLogic.hpp"

namespace
{
    Logger logger;
    SharedState sharedState;

    UltrasonicSensor ultrasonic(config::Pins::kUltrasonicTrig, config::Pins::kUltrasonicEcho);
    WaterSensor waterSensor(config::Pins::kWaterSensor, config::kWater.activeHigh);
    HallCurrentSensor currentSensor
    (
        config::Pins::kHallCurrentAdc,
        config::kCurrent.adcRefMv,
        config::kCurrent.adcMaxCounts,
        config::kCurrent.zeroCurrentMv,
        config::kCurrent.sensitivityMvPerA
    );

    PumpController pump(config::Pins::kPumpGate, true);
    ClimateController climate(logger, sharedState, -1, true);
    BuzzerController buzzer(config::Pins::kBuzzer, true);

    EthernetManager ethernetManager(logger, sharedState);
    E3dcModbusClient e3dc(logger);
    ControlLogic control(logger, sharedState, pump, climate, buzzer);
    WebApiServer webApi(logger, sharedState, control);

    TaskHandle_t sensorTaskHandle = nullptr;
    TaskHandle_t controlTaskHandle = nullptr;
    TaskHandle_t energyTaskHandle = nullptr;
    TaskHandle_t webTaskHandle = nullptr;
    TaskHandle_t buzzerTaskHandle = nullptr;

    float distanceToLevelMm(const float distanceMm)
    {
        const float level = config::kTank.sensorToBottomMm - distanceMm;
        if (level < 0.0F)
        {
            return 0.0F;
        }
        if (level > config::kTank.sensorToBottomMm)
        {
            return config::kTank.sensorToBottomMm;
        }
        return level;
    }

    void sensorTask(void *)
    {
        logger.info("task", "Sensor task started");

        while (true)
        {
            float distanceMm = 0.0F;
            const bool ultrasonicValid = ultrasonic.readDistanceMm(distanceMm) && distanceMm >= config::kTank.validMinMm && distanceMm <= config::kTank.validMaxMm;

            const float levelMm = ultrasonicValid ? distanceToLevelMm(distanceMm) : 0.0F;
            const bool overflowWet = waterSensor.isWet();
            const float currentA = currentSensor.readCurrentA();
            const bool currentDetected = currentA >= config::kCurrent.currentDetectedA;

            sharedState.update([&](SystemStateData &state)
            {
                state.sensors.ultrasonicValid = ultrasonicValid;
                state.sensors.distanceMm = distanceMm;
                state.sensors.levelMm = levelMm;
                state.sensors.overflowSensorWet = overflowWet;
                state.sensors.pumpCurrentA = currentA;
                state.sensors.pumpCurrentDetected = currentDetected;
                state.sensors.lastUpdateMs = millis();

                setFault(state.faultMask, kFaultUltrasonicInvalid, !ultrasonicValid); 
            });

            vTaskDelay(pdMS_TO_TICKS(config::kTask.sensorTaskMs));
        }
    }

    void controlTask(void *)
    {
        logger.info("task", "Control task started");
        while (true)
        {
            control.tick();
            vTaskDelay(pdMS_TO_TICKS(config::kTask.controlTaskMs));
        }
    }

    void energyTask(void *)
    {
        logger.info("task", "Energy task started");

        while (true)
        {
            if (!config::kEnergy.enabled)
            {
                vTaskDelay(pdMS_TO_TICKS(config::kEnergy.pollIntervalMs));
                continue;
            }

            E3dcTelemetry telemetry;
            const bool ok = e3dc.poll(config::kEnergy.e3dcIp, config::kEnergy.modbusPort, config::kEnergy.modbusUnitId, telemetry);

            sharedState.update([&](SystemStateData &state)
            {
                state.energy.connected = ok;
                state.energy.valid = ok && telemetry.valid;
                state.energy.lastUpdateMs = millis();

                if (ok && telemetry.valid) 
                {
                    state.energy.pvPowerW = telemetry.pvPowerW;
                    state.energy.batteryPowerW = telemetry.batteryPowerW;
                    state.energy.housePowerW = telemetry.housePowerW;
                    state.energy.gridPointPowerW = telemetry.gridPointPowerW;
                    state.energy.batterySocPercent = telemetry.batterySocPercent;
                   state.energy.emsStatus = telemetry.emsStatus;
                   setFault(state.faultMask, kFaultEnergyOffline, false);
                } 
                else 
                {
                    setFault(state.faultMask, kFaultEnergyOffline, true);
                } 
            });

            vTaskDelay(pdMS_TO_TICKS(config::kEnergy.pollIntervalMs));
        }
    }

    void webTask(void *)
    {
        logger.info("task", "Web task started");
        webApi.begin();

        while (true)
        {
            webApi.update();
            vTaskDelay(pdMS_TO_TICKS(config::kTask.webTaskMs));
        }
    }

    void buzzerTask(void *)
    {
        logger.info("task", "Buzzer task started");
        while (true)
        {
            buzzer.update();
            vTaskDelay(pdMS_TO_TICKS(config::kTask.buzzerTaskMs));
        }
    }

}

void setup()
{
    Serial.begin(115200);
    delay(300);

    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed");
    }

    logger.begin();
    logger.info("boot", "Starting ESP32-S3 climate condensate pump controller");

    ultrasonic.begin();
    waterSensor.begin();
    currentSensor.begin();
    pump.begin();
    climate.begin();
    buzzer.begin();

    ethernetManager.begin();

    xTaskCreatePinnedToCore(sensorTask, "sensorTask", 4096, nullptr, 2, &sensorTaskHandle, 1);
    xTaskCreatePinnedToCore(controlTask, "controlTask", 6144, nullptr, 3, &controlTaskHandle, 1);
    xTaskCreatePinnedToCore(energyTask, "energyTask", 6144, nullptr, 1, &energyTaskHandle, 0);
    xTaskCreatePinnedToCore(webTask, "webTask", 8192, nullptr, 1, &webTaskHandle, 0);
    xTaskCreatePinnedToCore(buzzerTask, "buzzerTask", 3072, nullptr, 1, &buzzerTaskHandle, 1);
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
