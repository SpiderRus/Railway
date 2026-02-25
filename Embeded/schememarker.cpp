#include <schememarker/schememarker.h>

#ifdef USE_SHEME_MARKER

#include <utils/Utils.h>

SchemeMarker::SchemeMarker(const uint8_t sensorPin, const bool reversed) noexcept: sensorPin(sensorPin), reversed(reversed), callback(nullptr), lastTime(0) {}

void IRAM_ATTR SchemeMarker::sensorInterrupt(void *arg) noexcept {
    SchemeMarker &self = *(SchemeMarker*) arg;
    const bool currentState = gpio_get_level(self.sensorPin);

    if (likely(currentState != self.lastState)) {
        const uint32_t time = currentMillis();
        self.lastState = currentState;

        if (currentState == !self.reversed)
            self.lastOnTime = time;
        else {
            const uint32_t diff = time - self.lastOnTime;

            if (likely(diff < 10000)) {
                self.lastTime = currentTimeMillis(time) - (diff >> 1);
            
                if (likely(self.callback))
                    self.callback(self.callbackArgs);
            }
        }
    }
}

void SchemeMarker::begin() noexcept {
    lastTime = 0;
    pinMode(sensorPin, INPUT_PULLUP);

    if (unlikely((lastState = gpio_get_level(sensorPin)) == !reversed))
        lastOnTime = currentMillis();
    else
        lastOnTime = 0;

    attachInterruptArg(sensorPin, sensorInterrupt, this, CHANGE);
}

#endif

