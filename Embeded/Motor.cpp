#include <motor/Motor.h>

#ifdef USE_MOTOR

#include <sys/_stdint.h>
#include "esp32-hal-gpio.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include <Arduino.h>
#include "esp32-hal-ledc.h"

#include <logger/logger.h>

static const char TAG[] = "MOTOR";

static const uint32_t signMask = 0x80000000;
static const uint32_t signMaskAnd = 0xAFFFFFFF;

Motor::Motor(uint8_t plusPwmPin, uint8_t minusPwmPin, uint16_t smoothMs, uint16_t smoothSteps, uint8_t res, uint32_t freq) noexcept
      : QueueWorker<float>(1, "MOTOR", NORMAL_TASK_PRIORITY),
        plusPwmPin(plusPwmPin),
        minusPwmPin(minusPwmPin),
        smoothMs(smoothMs),
        smoothSteps(smoothSteps),
        pause(smoothMs / smoothSteps),
        minValue(-((1 << res) - 1)), 
        maxValue((1 << res) - 1),
        resolution(res),
        frequency(freq),
        currentValue(0.0),
        direction(0),
        emergencyStop(false),
        K(((float)std::max(std::abs(maxValue), std::abs(minValue))) / smoothSteps) {}

void IRAM_ATTR Motor::processQueueItem(float& value) noexcept {
  rt:
    float current = currentValue;
    const float diff = value - current;

    if (likely(std::abs(diff) >= 1)) {
        const int direction = (int) sign(diff);
        const float step = direction * K;

        if (direction > 0) {
            while (current < value) {
                current += step;
                setValue(std::min(value, current));
                if (unlikely(receiveQueueItem(value, pause)))
                    goto rt;
            }
        } else {
            while (current > value) {
                current += step;
                setValue(std::max(value, current));
                if (unlikely(receiveQueueItem(value, pause)))
                    goto rt;
            }
        }
    }
}

void IRAM_ATTR Motor::setPwmValue(int16_t value) noexcept {
    if (likely(value != 0)) {
        if (unlikely(direction == 0)) {
            if (value > 0) {
                ledcAttach(currentPwmPin = plusPwmPin, frequency, resolution);
                gpio_set_level((gpio_num_t) minusPwmPin, LOW);
            } else {
                ledcAttach(currentPwmPin = minusPwmPin, frequency, resolution);
                gpio_set_level((gpio_num_t) plusPwmPin, LOW);
            }

            direction = sign(value);
        } else {
            if (direction > 0) {
                if (unlikely(value < 0)) {
                    ledcWrite(currentPwmPin, 0);
                    ledcDetach(currentPwmPin);
                    
                    gpio_set_level((gpio_num_t)plusPwmPin, LOW);
                    ledcAttach(currentPwmPin = minusPwmPin, frequency, resolution);

                    direction = -1;
                }
            } else {
                if (unlikely(value > 0)) {
                    ledcWrite(currentPwmPin, 0);

                    ledcDetach(minusPwmPin);
                    gpio_set_level((gpio_num_t)minusPwmPin, LOW);
                    ledcAttach(currentPwmPin = plusPwmPin, frequency, resolution);

                    direction = 1;
                }
            }
        }

        ledcWrite(currentPwmPin, value > 0 ? value : -value);
    } else {
        if (direction != 0) {
            ledcWrite(currentPwmPin, 0);

            ledcDetach(plusPwmPin);
            ledcDetach(minusPwmPin);

            gpio_set_level((gpio_num_t)plusPwmPin, LOW);
            gpio_set_level((gpio_num_t)minusPwmPin, LOW);
            
            direction = 0;  
        }
    }
}

void IRAM_ATTR Motor::setValue(float value) noexcept {
    if (unlikely(value < minValue))
        value = minValue;
    else if (unlikely(value > maxValue))
        value = maxValue;

    if (likely(std::abs(currentValue - value) >= 1)) {
        int16_t v = (int16_t) (currentValue = value);

        if (std::abs(v - value) >= 0.5 && value > minValue && value < maxValue)
            v += sign(value);

        setPwmValue(v);
    }
}

void Motor::begin(void) noexcept {
  pinMode(plusPwmPin, OUTPUT);
  gpio_set_level((gpio_num_t) plusPwmPin, LOW);

  pinMode(minusPwmPin, OUTPUT);
  gpio_set_level((gpio_num_t) minusPwmPin, LOW);
}

#endif