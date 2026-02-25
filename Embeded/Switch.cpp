#include <switch/Switch.h>

#ifdef USE_SWITCH

#include <Arduino.h>
#include <algorithm>
#include <stdlib.h>
#include <sys/_stdint.h>
#include "esp_compiler.h"

Switch::Switch(const char *name, uint16_t smoothMs, uint16_t smoothSteps, uint16_t maxValue, uint16_t stabValue, uint8_t res, uint32_t freq) noexcept
    : QueueWorker<bool>(1, name, NORMAL_TASK_PRIORITY),
      smoothMs(smoothMs),
      smoothSteps(smoothSteps),
      maxValue(maxValue),
      stabValue(stabValue),
      resolution(res),
      frequency(freq),
      pause(likely(smoothSteps > 0) ? smoothMs / smoothSteps : 0),
      step(likely(smoothSteps > 0) ? ((float)maxValue) / smoothSteps : (float)maxValue),
      high((1 << resolution) - 1) {}

SimpleSwitch::SimpleSwitch(bool releaseState, uint8_t pin, uint16_t smoothMs, uint16_t smoothSteps, 
            uint16_t maxValue, uint16_t stabValue, 
            uint8_t res, uint32_t freq) noexcept
    : Switch("SWITCH", smoothMs, smoothSteps, maxValue, stabValue, res, freq),
      releaseState(releaseState),
      pin(pin) {}

void IRAM_ATTR SimpleSwitch::processQueueItem(bool& value) noexcept {
    if (likely(value != currentValue)) {
      rt:
        const bool targetValue = value;

        if (targetValue == releaseState) {
            ledcWrite(pin, 0);

            currentValue = targetValue;
        } else {
            float curPwm = step;

            // плавно включаем
            ledcWrite(pin, (uint32_t)curPwm);
            while(likely(curPwm < maxValue)) {
                if (unlikely(receiveQueueItem(value, pause) && targetValue != value))
                    goto rt;

                curPwm = std::min(curPwm + step, (float) maxValue);
                ledcWrite(pin, (uint32_t) curPwm);
            }

            currentValue = targetValue;

            // пауза на переключение
            if (unlikely(receiveQueueItem(value, smoothMs) && targetValue != value))
                goto rt;

            // мощность удержания
            ledcWrite(pin, stabValue);
        }
    }
}

void SimpleSwitch::begin(void) noexcept {
    pinMode(pin, OUTPUT);
    ledcAttach(pin, frequency, resolution);

    currentValue = true;
    setValue(false);
}

BidirectionalSwitch::BidirectionalSwitch(uint8_t plusPwmPin, uint8_t minusPwmPin, uint16_t smoothMs, uint16_t smoothSteps, 
            uint16_t maxValue, uint16_t stabValue, uint8_t res, uint32_t freq) noexcept
    : Switch("BISWITCH", smoothMs, smoothSteps, maxValue, stabValue, res, freq),
      plusPwmPin(plusPwmPin),
      minusPwmPin(minusPwmPin) {}

void IRAM_ATTR BidirectionalSwitch::processQueueItem(bool& value) noexcept {
    if (likely(value != currentValue)) {
      rt:
        const bool targetValue = value;
        // stop
        ledcWrite(plusPwmPin, high);
        ledcWrite(minusPwmPin, high);
        if (unlikely(receiveQueueItem(value, smoothMs >> 1) && targetValue != value))
            goto rt;
        
        float curPwm = step;
        uint8_t pwmPin;

        if (targetValue) {
            pwmPin = plusPwmPin;
            ledcWrite(minusPwmPin, 0);
        } else {
            pwmPin = minusPwmPin;
            ledcWrite(plusPwmPin, 0);
        }
        ledcWrite(pwmPin, (uint32_t)curPwm);
        
        while(likely(curPwm < maxValue)) {
            if (unlikely(receiveQueueItem(value, pause) && targetValue != value))
                goto rt;
            curPwm = std::min(curPwm + step, (float)maxValue);
            ledcWrite(pwmPin, (uint32_t)curPwm);
        }

        currentValue = targetValue;
    
        if (unlikely(receiveQueueItem(value, smoothMs) && targetValue != value))
            goto rt;

        ledcWrite(pwmPin, stabValue);
    }
}

void BidirectionalSwitch::begin(void) noexcept {
    pinMode(plusPwmPin, OUTPUT);
    pinMode(minusPwmPin, OUTPUT);

    ledcAttach(plusPwmPin, frequency, resolution);
    ledcWrite(plusPwmPin, 0);

    ledcAttach(minusPwmPin, frequency, resolution);
    ledcWrite(minusPwmPin, 0);

    currentValue = true;
    setValue(false);
}

#endif
