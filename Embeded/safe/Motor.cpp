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

#include "Motor.hpp"
#include "logger.hpp"

static const char MOTOR_LOG[] = "MOTOR";

static const uint32_t signMask = 0x80000000;
static const uint32_t signMaskAnd = 0xAFFFFFFF;

Motor::Motor(Configuration& conf, uint16_t res, uint32_t stepUs, uint16_t channel, uint32_t freq) noexcept
      : minValue(-((1 << res) - 1)), 
        maxValue((1 << res) - 1),
        resolution(res),
        stepTimeUs(stepUs),
        frequency(freq),
        pwmChannel(channel),
        targetValue(0),
        currentValue(0),
        state(MOTION),
        step(0),
        started(false),
        direction(0),
        config(conf),
        emergencyStop(false),
        zeroCallback(NULL) {
}

void Motor::timerFunc(void *data) noexcept {
  Motor& self = *(Motor*)data;

  CRITICAL(self.mux, {
    switch(self.state) {
      case MOTION: {
        int32_t step = self.step;

        if (step == 0)
          return;

        const int32_t currentValue = self.currentValue;
        const int16_t currentValueRounded = (int16_t) rotateRightSign(currentValue, 16);
        int32_t nextValue = currentValue + step;
        int16_t nextValueRounded = (int16_t) rotateRightSign(nextValue, 16);

        if (nextValueRounded != currentValueRounded) {
          // detect zero cross
          if (nextValue == 0 || (currentValue != 0 && sign(currentValue) != sign(nextValue))) {
            // breaking 
            //SP_LOGI(MOTOR_LOG, "Breaks on");

            nextValue = 0;
            self.setPwmValue(0);
            digitalWrite(self.config.getMotorConfig().plusPin, HIGH);
            digitalWrite(self.config.getMotorConfig().minusPin, HIGH);

            self.state = BREAKING;
          }
          else {
            if (nextValueRounded < self.minValue)
              nextValueRounded = self.minValue;
            else if (nextValueRounded > self.maxValue)
              nextValueRounded = self.maxValue;

            const int16_t targetValue = self.targetValue;
            if ((nextValueRounded <= targetValue && targetValue <= currentValueRounded) || 
                ((nextValueRounded >= targetValue && targetValue >= currentValueRounded))) { 
              nextValue = rotateLeftSign(targetValue, 16);
              nextValueRounded = targetValue;

              //SP_LOGI(MOTOR_LOG, "PWM end. PWM: %d", (int)nextValueRounded);

                if (self.step == step) {
                  self.step = 0;

                  esp_timer_stop(self.timerHandle);
                  self.started = false;
                }
            }

            self.setPwmValue(nextValueRounded);
          }
        }

        self.currentValue = nextValue;
        break;
      }

      case BREAKING:
        if (!self.isBreaking()) {
          // turn off breaking
          //SP_LOGI(MOTOR_LOG, "Breaks released");

          digitalWrite(self.config.getMotorConfig().plusPin, LOW);
          digitalWrite(self.config.getMotorConfig().minusPin, LOW);

          self.state = MOTION;
        }
        break;  
    }
  });
}

bool Motor::isBreaking(void) noexcept {
  return zeroCallback && !zeroCallback();
}

void Motor::setPwmValue(int16_t value) noexcept {
  const uint8_t plusPwmPin = config.getMotorConfig().plusPin;
  const uint8_t minusPwmPin = config.getMotorConfig().minusPin;

  if (value != 0) {
    if (direction == 0) {
      if (value > 0) {
        //ESP_LOGI(MOTOR_LOG, "Start forward");

        ledcAttachPin(plusPwmPin, pwmChannel);
        digitalWrite(minusPwmPin, LOW);
      }
      else {
        //ESP_LOGI(MOTOR_LOG, "Start backward");

        ledcAttachPin(minusPwmPin, pwmChannel);
        digitalWrite(plusPwmPin, LOW);
      }

      direction = sign(value);
    }
    else {
      if (direction > 0) {
        if (value < 0) {
          ledcWrite(pwmChannel, 0);

          ledcDetachPin(plusPwmPin);
          digitalWrite(plusPwmPin, LOW);
          ledcAttachPin(minusPwmPin, pwmChannel);

          direction = -1;
        }
      }
      else {
        if (value > 0) {
          ledcWrite(pwmChannel, 0);

          ledcDetachPin(minusPwmPin);
          digitalWrite(minusPwmPin, LOW);
          ledcAttachPin(plusPwmPin, pwmChannel);

          direction = 1;
        }
      }
    }

    ledcWrite(pwmChannel, abs(value));  
  }
  else {
    if (direction != 0) {
      ledcWrite(pwmChannel, 0);

      if (direction > 0)
        ledcDetachPin(plusPwmPin);
      else
        ledcDetachPin(minusPwmPin);

      digitalWrite(plusPwmPin, LOW);
      digitalWrite(minusPwmPin, LOW);

      direction = 0;  
    }
  }
}

void Motor::recalc(const int16_t value) noexcept {
  while(!emergencyStop) {
    const int16_t oldTarget = targetValue;
    const int32_t deltaValue = value - rotateRightSign(currentValue, 16);
    
    const uint64_t d = (((uint64_t)config.getMotorConfig().fullPowerDelayMs) * 2000) * (uint64_t)abs(deltaValue);
    const uint32_t v = stepTimeUs * (uint32_t)abs(maxValue - minValue);
    
    const uint32_t steps =  (uint32_t) (d / v);

    const int32_t s = steps ? (rotateLeftSign(deltaValue, 16) / (int32_t)steps) : 0;
      
    CRITICAL(mux, {
      if (!emergencyStop && oldTarget == targetValue) {
        targetValue = value;
        step = s;

        if (s != 0 && !started) {
          esp_timer_start_periodic(timerHandle, stepTimeUs);
          started = true;
        }

        break;
      }
    });
  }
    //SP_LOGI(MOTOR_LOG, "Set value=%d, step=%d, steps=%u, deltaValue=%d", (int)value, s, steps, deltaValue);
}

void Motor::recalcAsDouble(const double value) noexcept {
  while(!emergencyStop) {
    const int16_t oldTarget = targetValue;
    int16_t t = (int16_t)value;
    if (abs(value - t) >= 0.5)
      t += t < 0 ? -1 : 1;

    const double deltaValue = t - getCurrentValueAsDouble();
    
    const double d = (((double)config.getMotorConfig().fullPowerDelayMs) * 2000.0) * abs(deltaValue);
    const uint32_t v = stepTimeUs * (uint32_t)abs(maxValue - minValue);
    
    const double steps =  d / v;

    const int32_t s = steps >= 0.001 ? (int32_t)((deltaValue * 65536.0) / steps) : 0;

    CRITICAL(mux, {     
      if (!emergencyStop && oldTarget == targetValue) {
        targetValue = t;
        step = s;

        if (s != 0 && !started) {
          esp_timer_start_periodic(timerHandle, stepTimeUs);
          started = true;
        }

        //SP_LOGI(MOTOR_LOG, "Set value=%f, step=%d, steps=%f, deltaValue=%f", value, s, steps, deltaValue);
        break;
      }
    });
  }
}

void Motor::setValue(int16_t value) noexcept {
  if (value < minValue)
    value = minValue;
  else if (value > maxValue)
    value = maxValue;

  if (value != targetValue) {
    recalc(value);
  }
}

void Motor::setValueAsDouble(double value) noexcept {
  if (value < minValue)
    value = minValue;
  else if (value > maxValue)
    value = maxValue;

  if (abs(value - targetValue) > 0.001) {
    recalcAsDouble(value);
  }
}

void Motor::begin(void) noexcept {
  esp_timer_create_args_t timer_args;
  timer_args.callback = &timerFunc;
  timer_args.name = "motor";
  timer_args.arg = this;
  timer_args.dispatch_method = ESP_TIMER_TASK;

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timerHandle));

  const uint8_t plusPwmPin = config.getMotorConfig().plusPin;
  const uint8_t minusPwmPin = config.getMotorConfig().minusPin;

  pinMode(plusPwmPin, OUTPUT);
  digitalWrite(plusPwmPin, LOW);

  pinMode(minusPwmPin, OUTPUT);
  digitalWrite(minusPwmPin, LOW);

  ledcSetup(pwmChannel, frequency, resolution);
}

void Motor::setEmergencyStop(bool value) noexcept {
  const uint8_t plusPwmPin = config.getMotorConfig().plusPin;
  const uint8_t minusPwmPin = config.getMotorConfig().minusPin;

  TRY_CRITICAL(mux, {
    if (value != emergencyStop) {
      if (value) {
        targetValue = 0;
        currentValue = 0;
        step = 0;
        direction = 0;
        state = State::MOTION;
        ledcDetachPin(minusPwmPin);
        ledcDetachPin(plusPwmPin);
        digitalWrite(plusPwmPin, HIGH);
        digitalWrite(minusPwmPin, HIGH);

        if (started) {
          esp_timer_stop(timerHandle);
          started = false;
        }
      }
      else {
        digitalWrite(plusPwmPin, LOW);
        digitalWrite(minusPwmPin, LOW);
      }

      emergencyStop = value;
    }
  });
}
