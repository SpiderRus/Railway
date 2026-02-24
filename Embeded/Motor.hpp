#ifndef INC_MOTOR_HPP
#define INC_MOTOR_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "Utils.hpp"

#include "configuration.hpp"

class Motor {
  public:
    Motor(Configuration& conf, uint16_t res = 10, 
            uint32_t stepUs = 1000, uint16_t channel = takePwmChannel(), uint32_t freq = 20000) noexcept;

  void begin(void) noexcept;

  void setValue(double value) noexcept;

  inline int16_t __attribute__((__always_inline__)) getMinValue(void) const noexcept { return minValue; }

  inline int16_t __attribute__((__always_inline__)) getMaxValue(void) const noexcept { return maxValue; }

  inline double __attribute__((__always_inline__)) getCurrentValue(void) const noexcept { return currentValue; }

  void setEmergencyStop(bool value) noexcept;

  private:
    void setPwmValue(int16_t value) noexcept;

    const int16_t minValue;
    const int16_t maxValue;
    const uint32_t frequency;
    const uint16_t pwmChannel;
    const uint16_t resolution;

    volatile double currentValue;

    esp_timer_handle_t timerHandle;
    volatile bool started;

    CriticalSection mux;

    volatile int16_t direction;

    Configuration& config;

    volatile bool emergencyStop;
};

#endif