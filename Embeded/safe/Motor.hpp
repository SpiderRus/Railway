#ifndef INC_MOTOR_HPP
#define INC_MOTOR_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "Utils.hpp"

#include "configuration.hpp"

enum State {
  MOTION,
  BREAKING
};

inline int32_t __attribute__((__always_inline__)) rotateRightSign(int32_t value, uint bits) noexcept {
  const int32_t result = value >> bits;

  return value < 0 ? (result | 0x80000000) : (result & 0xAFFFFFFF);
}

inline int32_t __attribute__((__always_inline__)) rotateLeftSign(int32_t value, uint bits) noexcept {
  const int32_t result = value << bits;

  return value < 0 ? (result | 0x80000000) : (result & 0xAFFFFFFF);
}

class Motor {
  public:
    Motor(Configuration& conf, uint16_t res = 10, 
            uint32_t stepUs = 1000, uint16_t channel = 0, uint32_t freq = 20000) noexcept;

  void begin(void) noexcept;

  void setValue(int16_t value) noexcept;

  void setValueAsDouble(double value) noexcept;

  inline uint16_t __attribute__((__always_inline__)) getFullPowerDelay(void) const noexcept { return config.getMotorConfig().fullPowerDelayMs; }

  inline int16_t __attribute__((__always_inline__)) getMinValue(void) const noexcept { return minValue; }

  inline int16_t __attribute__((__always_inline__)) getMaxValue(void) const noexcept { return maxValue; }

  inline int16_t __attribute__((__always_inline__)) getCurrentValue(void) const noexcept { return (int16_t) rotateRightSign(currentValue, 16); }

  inline double __attribute__((__always_inline__)) getCurrentValueAsDouble(void) const noexcept { return currentValue / 65536.0; }

  inline int16_t __attribute__((__always_inline__)) getTargetValue(void) const noexcept { return targetValue; };

  void setEmergencyStop(bool value) noexcept;

  inline void __attribute__((__always_inline__)) setZeroCallback(std::function<bool()> zCallback) noexcept { zeroCallback = zCallback; };

  private:
    static void timerFunc(void*) noexcept;
    void recalc(int16_t value) noexcept;

    void recalcAsDouble(double value) noexcept;

    bool isBreaking(void) noexcept;

    void setPwmValue(int16_t value) noexcept;

    const int16_t minValue;
    const int16_t maxValue;
    const uint32_t stepTimeUs;
    const uint32_t frequency;
    const uint16_t pwmChannel;
    const uint16_t resolution;

    std::function<bool()> zeroCallback;

    volatile int16_t targetValue;
    volatile int32_t currentValue;
    volatile State state;

    volatile int32_t step;

    esp_timer_handle_t timerHandle;
    volatile bool started;

    CriticalSection mux;

    volatile int16_t direction;

    Configuration& config;

    volatile bool emergencyStop;
};

#endif