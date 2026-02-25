#ifndef INC_MOTOR_H
#define INC_MOTOR_H

#include <config/config.h>

#ifdef USE_MOTOR

#include <cmath>
#include "esp_timer.h"
#include "esp_system.h"
#include <utils/Utils.h>
#include <utils/QueueWorker.h>

class Motor : protected QueueWorker<float> {
  public:
    Motor(uint8_t plusPwmPin, uint8_t minusPwmPin, uint16_t smoothMs, uint16_t smoothSteps, uint8_t res = 10, uint32_t freq = 30000) noexcept;

    void begin(void) noexcept;

    void IRAM_ATTR setValue(float value) noexcept;

    inline void __attribute__((__always_inline__)) setValueSmoothly(float value) noexcept {
        overwriteQueueItem(value);
    }

    inline int16_t __attribute__((__always_inline__)) getMinValue(void) const noexcept { return minValue; }

    inline int16_t __attribute__((__always_inline__)) getMaxValue(void) const noexcept { return maxValue; }

    inline float __attribute__((__always_inline__)) getCurrentValue(void) const noexcept { return currentValue; }

    inline void __attribute__((__always_inline__)) setPowerSmoothly(float power) noexcept {
        if (power >= 0)
            setValueSmoothly(std::round(maxValue * std::min(power, 100.0f) * 10.0) / 1000.0);
        else
            setValueSmoothly(std::round(minValue * std::min(-power, 100.0f) * 10.0) / 1000.0);
    }

    inline float __attribute__((__always_inline__)) getPower() const noexcept {
        const auto value = currentValue;

        return std::round(100000.0 * (value >= 0 ? (value / maxValue) : (-value / minValue))) / 1000.0;
    }

    inline int __attribute__((__always_inline__)) getDirection(void) const noexcept { return signNonZero(direction); }

    void setEmergencyStop(bool value) noexcept;

    void IRAM_ATTR setPwmValue(int16_t value) noexcept;

  protected:
    void IRAM_ATTR processQueueItem(float& data) noexcept override;

  private:
    const uint8_t plusPwmPin, minusPwmPin, resolution;

    const int16_t minValue, maxValue, smoothMs, smoothSteps, pause;
    const uint32_t frequency;
    const float K;

    volatile uint8_t currentPwmPin;

    volatile float currentValue;

    volatile int direction;

    volatile bool emergencyStop;
};

#endif

#endif // INC_MOTOR_H