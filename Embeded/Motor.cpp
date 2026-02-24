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

#include "Utils.hpp"
#include "Motor.hpp"
#include "logger.hpp"

static const char MOTOR_LOG[] = "MOTOR";

static const uint32_t signMask = 0x80000000;
static const uint32_t signMaskAnd = 0xAFFFFFFF;

Motor::Motor(Configuration& conf, uint16_t res, uint32_t stepUs, uint16_t channel, uint32_t freq) noexcept
      : minValue(-((1 << res) - 1)), 
        maxValue((1 << res) - 1),
        resolution(res),
        frequency(freq),
        pwmChannel(channel),
        currentValue(0.0),
        direction(0),
        config(conf),
        emergencyStop(false) {
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

      ledcDetachPin(plusPwmPin);
      ledcDetachPin(minusPwmPin);

      digitalWrite(plusPwmPin, LOW);
      digitalWrite(minusPwmPin, LOW);

      direction = 0;  
    }
  }
}

void Motor::setValue(double value) noexcept {
  if (value < minValue)
    value = minValue;
  else if (value > maxValue)
    value = maxValue;

  if (abs(currentValue - value) >= 0.001) {
    int16_t v = (int16_t) (currentValue = value);

    if (abs(v - value) >= 0.5 && value > minValue && value < maxValue)
      v += sign(value);

    if (abs(v) < config.getMotorConfig().minPower)
      v = 0;

    setPwmValue(v);
  }
}

void Motor::begin(void) noexcept {
  const uint8_t plusPwmPin = config.getMotorConfig().plusPin;
  const uint8_t minusPwmPin = config.getMotorConfig().minusPin;

  pinMode(plusPwmPin, OUTPUT);
  digitalWrite(plusPwmPin, LOW);

  pinMode(minusPwmPin, OUTPUT);
  digitalWrite(minusPwmPin, LOW);

  ledcSetup(pwmChannel, frequency, resolution);
}

void Motor::setEmergencyStop(bool value) noexcept {
}
