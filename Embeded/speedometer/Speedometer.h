#include <atomic>
#ifndef __SPEEDOMETER_H
#define __SPEEDOMETER_H

#include <config/config.h>

#ifdef USE_SPEEDOMETER

#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <utils/Utils.h>

#define SPEEDOMETER_THRESHOLD_US 200000

class Speedometer {
public:
    Speedometer(gpio_num_t sensorPin, float wheelLenght) noexcept : sensorPin(sensorPin), wheelLenght(wheelLenght) {}

    inline float __attribute__((__always_inline__)) getRPS() const noexcept {
        const uint64_t now = esp_timer_get_time();
        const uint64_t diff = diffInterruptUs;

        return unlikely(now - lastInterruptUs > SPEEDOMETER_THRESHOLD_US || diff == 0) ? 0.0f : (500000.0f / diff);
    }

    // mm/sec
    inline float __attribute__((__always_inline__)) getSpeed() const noexcept {
        return wheelLenght * getRPS();
    }

    inline size_t __attribute__((__always_inline__)) getRotations() const noexcept {
        return numInterrupts >> 1;
    }

    void begin() noexcept;

private:
    static void IRAM_ATTR sensorInterrupt(void *arg) noexcept;

    const gpio_num_t sensorPin;
    const float wheelLenght;

    std::atomic_uint64_t lastInterruptUs;
    std::atomic_uint64_t diffInterruptUs;
    volatile uint32_t numInterrupts;
    volatile bool lastState;
};

#endif

#endif // __SPEEDOMETER_H