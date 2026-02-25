#include "utils/Utils.h"
#include <speedometer/speedometer.h>

#ifdef USE_SPEEDOMETER

void IRAM_ATTR Speedometer::sensorInterrupt(void *arg) noexcept {
    Speedometer &self = *(Speedometer*) arg;
    const bool currentState = gpio_get_level(self.sensorPin);

    if (likely(currentState != self.lastState)) {
        const uint64_t now = esp_timer_get_time();
        const uint64_t diff = now - self.lastInterruptUs;

        self.diffInterruptUs = unlikely(diff > SPEEDOMETER_THRESHOLD_US) ? 0 : diff;

        self.lastInterruptUs = now;
        self.numInterrupts++;
        self.lastState = currentState;
    }
}

void Speedometer::begin() noexcept {
    lastInterruptUs = 0;
    diffInterruptUs = 0;
    numInterrupts = 0;

    pinMode(sensorPin, INPUT_PULLUP);
    lastState = gpio_get_level(sensorPin);
    attachInterruptArg(sensorPin, sensorInterrupt, this, CHANGE);
}

#endif
