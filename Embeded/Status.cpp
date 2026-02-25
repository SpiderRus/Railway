#include "esp_compiler.h"
#include "HardwareSerial.h"
#include <cstdint>
#include "semaphore/Semaphore.h"
#include "switch/Switch.h"
#include <status/Status.h>
#include <utils/Utils.h>

#define MIN_SEND_PERIOD_MS 200

Status::Status(
    #ifdef USE_MOTOR
        Motor& motor, 
    #endif

    #ifdef USE_SPEEDOMETER
        Speedometer& speedometer,
    #endif

    #ifdef USE_SHEME_MARKER
        SchemeMarker& schemeMarker,
    #endif

    #ifdef USE_SWITCH
        Switch& rSwitch,
    #endif

    #ifdef USE_SEMAPHORE
        Semaphores &semaphores,
    #endif

    #ifdef WITH_LEDS
        Led **leds,
        size_t ledsCount,
    #endif

    UDPSender& udpSender) noexcept :
            #ifdef USE_MOTOR
                motor(motor),
            #endif

            #ifdef USE_SPEEDOMETER
                speedometer(speedometer),
            #endif

            #ifdef USE_SHEME_MARKER
                schemeMarker(schemeMarker),
            #endif

            #ifdef USE_SWITCH
                rSwitch(rSwitch),
            #endif

            #ifdef USE_SEMAPHORE
                semaphores(semaphores),
            #endif

            #ifdef WITH_LEDS
                leds(leds), ledsCount(ledsCount),
            #endif

            udpSender(udpSender), version(0) {
    #ifdef USE_INTERNAL_TEMP
        tempHandle = nullptr;
        temperature_sensor_config_t temp_sensor = {
            .range_min = -10,
            .range_max = 80,
        };
        temperature_sensor_install(&temp_sensor, &tempHandle);
        temperature_sensor_enable(tempHandle);
    #endif

    #ifdef USE_SEMAPHORE
        semaphoresCount = semaphores.count();
        semaphoresSerializeBuffer = new uint32_t[semaphoresCount];
    #endif

    #ifdef WITH_LEDS
        ledColors = new uint8_t[ledsCount];
    #endif
}

#ifdef USE_SHEME_MARKER
    void IRAM_ATTR Status::schemeMarkerCallback(void *args) noexcept {
        Status& self = *(Status*) args;

        self.lastMarkerTime = self.schemeMarker.getLastTime();
        self.udpSender.sendFromISR(self.statusSendCallback, &self);
    }
#endif

#ifdef USE_INTERNAL_TEMP
    float Status::getInternalTemp() const noexcept {
        float result = 0;
        temperature_sensor_get_celsius(tempHandle, &result);
        return result;
    }
#endif

void Status::begin() noexcept {
    #ifdef USE_SHEME_MARKER
        schemeMarker.setCallback(schemeMarkerCallback, (void*) this);
        lastMarkerTime = schemeMarker.getLastTime();
    #endif

    #ifdef USE_INTERNAL_TEMP
        internalTemp = getInternalTemp();
    #endif

    #ifdef USE_MOTOR
        power = motor.getPower();
        direction = 0;
    #endif

    #ifdef USE_SPEEDOMETER
        speed = speedometer.getSpeed();
    #endif

    #ifdef USE_SWITCH
        switchState = rSwitch.getValue();
    #endif

    #ifdef USE_SEMAPHORE
        semaphoresChanged();
    #endif

    #ifdef WITH_LEDS
        ledsChanged();
    #endif

    version = 0;

    xTaskCreate(taskStatic, "STATUS", 4096, this, STATUS_TASK_PRIORITY, &taskHandle);
}

void Status::taskStatic(void *args) noexcept {
    ((Status*)args)->task();
}

void Status::task() noexcept {
    uint32_t prevTime = 0;

    do {
        const uint32_t timeMs = currentMillis();
        const uint32_t diff = timeMs - prevTime;

        if (diff >= 5000) {
            prevTime = timeMs;
            sendAllStatus();
        } else {
            #ifdef USE_INTERNAL_TEMP
                const float internalTemp = getInternalTemp();
            #endif

            #ifdef USE_MOTOR
                const float power = motor.getPower();
                const int direction = motor.getDirection();
            #endif

            #ifdef USE_SPEEDOMETER
                const float speed = speedometer.getSpeed();
            #endif

            #ifdef USE_SHEME_MARKER
                const uint64_t lastMarkerTime = schemeMarker.getLastTime();
            #endif

            #ifdef USE_SWITCH
                const bool switchState = rSwitch.getValue();
            #endif

            if (false
                    #ifdef USE_INTERNAL_TEMP
                        || (diff >= 2000 && !equals(internalTemp, this->internalTemp, 0.1f))
                    #endif
                    #ifdef USE_SPEEDOMETER
                        || !equals(speed, this->speed)
                    #endif
                    #ifdef USE_MOTOR
                        || !equals(power, this->power) || direction != this->direction
                    #endif
                    #ifdef USE_SHEME_MARKER
                        || lastMarkerTime != this->lastMarkerTime
                    #endif
                    #ifdef USE_SWITCH
                        || switchState != this->switchState
                    #endif
                    #ifdef USE_SEMAPHORE
                        || semaphoresChanged()
                    #endif
                    #ifdef WITH_LEDS
                        || ledsChanged()
                    #endif
                    ) {

                // Serial.printf("%d - %d (%d) Flags: %d, %d, %d, %d\n",
                //     (int) (prevTime % 1000000),
                //     (int) (timeMs % 1000000),
                //     (int) diff,
                //     (diff >= 2000 && !equals(internalTemp, this->internalTemp, 0.1f)) ? 1 : 0,
                //     !equals(speed, this->speed) ? 1 : 0,
                //     !equals(power, this->power) || direction != this->direction ? 1 : 0,
                //     lastMarkerTime != this->lastMarkerTime ? 1 : 0
                // );                        

                #ifdef USE_INTERNAL_TEMP
                    this->internalTemp = internalTemp;
                #endif

                #ifdef USE_SPEEDOMETER
                    this->speed = speed;
                #endif

                #ifdef USE_MOTOR
                    this->power = power;
                    this->direction = direction;
                #endif

                #ifdef USE_SHEME_MARKER
                    this->lastMarkerTime = lastMarkerTime;
                #endif

                #ifdef USE_SWITCH
                    this->switchState = switchState;
                #endif

                prevTime = timeMs;
                udpSender.send(statusSendCallback, (void*) this);
            }
        }

        taskDelay(((int32_t)MIN_SEND_PERIOD_MS) - (currentMillis() - timeMs));        
    } while (true);
}

uint16_t IRAM_ATTR Status::statusSendCallback(void *args, uint8_t *buffer, size_t maxLen) noexcept {
    Status& self = *(Status*) args;

    const uint64_t currentTimeMs = currentTimeMillis();

    // time is not sync. Ignore
    if (unlikely(currentTimeMs < ZERO_TIME_THRESHOLD_SEC * 1000))
        return 0;

    #ifdef USE_SEMAPHORE
        self.semaphoresChanged();
    #endif

    #ifdef WITH_LEDS
        self.ledsChanged();
    #endif

    return (int16_t) BinarySerializer::serialize(buffer, maxLen,
            (uint8_t) PACKET_TYPE_STATUS,
            (uint64_t) currentTimeMs,
            (uint32_t) ++self.version

            #ifdef USE_INTERNAL_TEMP
                , (int32_t) multBy1000(self.internalTemp)
            #else
                , (int32_t) 0
            #endif

            #ifdef USE_MOTOR
                #ifdef USE_SPEEDOMETER
                    , (int16_t) (self.speed * self.direction)
                #endif
                , (int32_t) multBy1000(self.power)
            #else
                #ifdef USE_SPEEDOMETER
                    , (int16_t) self.speed
                #endif
            #endif

            #ifdef USE_SHEME_MARKER
                , (uint64_t) self.lastMarkerTime
            #endif

            #ifdef USE_SWITCH
                , (uint8_t) self.switchState
            #endif

            #ifdef USE_SEMAPHORE
                , uint32Array(self.semaphoresSerializeBuffer, self.semaphoresCount)
            #endif

            #ifdef WITH_LEDS
                , uint8Array(self.ledColors, self.ledsCount)
            #endif
            );
}

void Status::sendAllStatus() noexcept {
    #ifdef USE_INTERNAL_TEMP
        internalTemp = getInternalTemp();
    #endif

    #ifdef USE_SPEEDOMETER
        speed = speedometer.getSpeed();
    #endif

    #ifdef USE_MOTOR
        power = motor.getPower();
        direction = motor.getDirection();
    #endif

    #ifdef USE_SHEME_MARKER
        lastMarkerTime = schemeMarker.getLastTime();
    #endif

    #ifdef USE_SWITCH
        switchState = rSwitch.getValue();
    #endif

    udpSender.send(statusSendCallback, (void*) this);
}
