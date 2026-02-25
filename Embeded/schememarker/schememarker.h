#ifndef INC_SCHEMARKER_HPP
#define INC_SCHEMARKER_HPP

#include <config/config.h>

#ifdef USE_SHEME_MARKER

#include <Arduino.h>

typedef std::function<void(void *args)> SchemeMarkerCallbackFunc;

class SchemeMarker {
public:
    SchemeMarker(const uint8_t gpio_num_t, const bool reversed) noexcept;

    void begin(void) noexcept;

    inline void __attribute__((__always_inline__)) setCallback(SchemeMarkerCallbackFunc callback, void *args) noexcept {
        this->callback = callback;
        this->callbackArgs = args;
    }

    inline uint64_t __attribute__((__always_inline__)) getLastTime(void) const noexcept {
        return lastTime;
    }

private:
    const gpio_num_t sensorPin;
    const bool reversed;
    SchemeMarkerCallbackFunc callback;
    void * volatile callbackArgs;

    volatile bool lastState;
    volatile uint32_t lastOnTime;
    volatile uint64_t lastTime;

    static void IRAM_ATTR sensorInterrupt(void *arg) noexcept;
};

#endif

#endif // INC_SCHEMARKER_HPP

