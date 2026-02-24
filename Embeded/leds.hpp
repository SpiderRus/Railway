#ifndef INC_MQTT_HPP
#define INC_MQTT_HPP

#include <Arduino.h>
#include "esp32-hal-ledc.h"

#include "Utils.hpp"

#pragma pack(push, 1)
typedef struct _PWM_LED_CONFIG {
  uint32_t frequency;
  uint8_t resolution;
  uint8_t pin;
} PWM_LED_CONFIG;

typedef struct _RGB_PWM_LED_CONFIG {
  PWM_LED_CONFIG r;
  PWM_LED_CONFIG g;
  PWM_LED_CONFIG b;
} RGB_PWM_LED_CONFIG;

#pragma pack(pop)

class Led {
  public:
    inline void __attribute__((__always_inline__)) setValue(double value) noexcept {
      setValue(convert(value));
    }

    inline void __attribute__((__always_inline__)) setValue(double r, double g, double b) noexcept {
      setValue(convert(r), convert(g), convert(b));
    }

    virtual void setValue(uint16_t value) noexcept;
    virtual void setValue(uint16_t r, uint16_t g, uint16_t b) noexcept;

  protected:
    inline uint16_t __attribute__((__always_inline__)) convert(double value) {
      return unlikely(value <= 0) ? 0 : ( unlikely(value >= 1.0) ? 0xFFFF : (uint16_t) (0xFFFF * value) );
    }
};

class PwmLed : public Led {
  public:
    PwmLed() noexcept : channel(0xFF) {
    } 

    PwmLed(PWM_LED_CONFIG& conf) noexcept : channel(takePwmChannel()), config(conf) {
    } 

    virtual void setValue(uint16_t value) noexcept {
      value = value >> (16 - config.resolution);

      if (likely(value != currentValue))
        ledcWrite(channel, currentValue = value);
    }

    virtual void setValue(uint16_t r, uint16_t g, uint16_t b) noexcept {
      setValue(std::max(r, std::max(g, b)));
    }

    PwmLed(uint8_t pin) noexcept : channel(takePwmChannel()) {
      config.pin = pin;
      config.frequency = 20000;
      config.resolution = 10;
    } 

    void begin() noexcept {
      setup();
    }

    void setConfig(PWM_LED_CONFIG& conf) noexcept {
      if (!memcmp(&conf, &config, sizeof(PWM_LED_CONFIG))) {
        ledcDetachPin(config.pin);
        pinMode(config.pin, INPUT);
        memcpy(&config, &conf, sizeof(PWM_LED_CONFIG));
        setup();
        ledcWrite(channel, currentValue);
      }
    }

  private:
    void setup() noexcept {
      pinMode(config.pin, OUTPUT);
      digitalWrite(config.pin, LOW);
      ledcSetup(channel, config.frequency, config.resolution);
      ledcAttachPin(config.pin, channel);
    }

    const uint8_t channel;
    PWM_LED_CONFIG config;
    volatile uint16_t currentValue = 0;

    static constexpr double k = 1.0 / (3.0 * 0xFFFFFF);
};

class RGBPwmLed : public Led {
  public:
    RGBPwmLed(uint8_t rPin, uint8_t gPin, uint8_t bPin) noexcept : r_v(rPin), g_v(gPin), b_v(bPin), r(r_v), g(g_v), b(b_v) {
    }

    RGBPwmLed(PwmLed& rLed, PwmLed& gLed, PwmLed& bLed) noexcept : r(rLed), g(gLed), b(bLed) {
    }

    void begin() noexcept {
      r.begin();
      g.begin();
      b.begin();
    }

    virtual void setValue(uint16_t value) noexcept {
      r.setValue(value);
      g.setValue(value);
      b.setValue(value);
    }

    virtual void setValue(uint16_t r, uint16_t g, uint16_t b) noexcept {
      this->r.setValue(r);
      this->g.setValue(g);
      this->b.setValue(b);
    }


    void setConfig(RGB_PWM_LED_CONFIG& conf) noexcept {
      r.setConfig(conf.r);
      g.setConfig(conf.g);
      b.setConfig(conf.b);
    }

  private:
    PwmLed& r;
    PwmLed r_v;
    PwmLed& g;
    PwmLed g_v;
    PwmLed& b;
    PwmLed b_v;
};

enum RotationDirection {
  NONE, LEFT, RIGHT
};

class BackLights {
  public:
    inline void __attribute__((__always_inline__)) setDirection(bool forward) noexcept {
      if (likely(forward)) {
        if (unlikely(nigthLigth)) {
          if (likely(top != nullptr)) top->setValue((uint16_t) 0x7FFF, (uint16_t) 0, (uint16_t) 0);
          if (likely(left != nullptr && rDirection != RotationDirection::LEFT)) left->setValue((uint16_t) 0x7FFF, (uint16_t) 0, (uint16_t) 0);
          if (likely(right != nullptr && rDirection != RotationDirection::RIGHT)) right->setValue((uint16_t) 0x7FFF, (uint16_t) 0, (uint16_t) 0);
        }
        else {
          if (likely(top != nullptr)) top->setValue((uint16_t) 0, (uint16_t) 0, (uint16_t) 0);
          if (likely(left != nullptr && rDirection != RotationDirection::LEFT)) left->setValue((uint16_t) 0, (uint16_t) 0, (uint16_t) 0);
          if (likely(right != nullptr) && rDirection != RotationDirection::RIGHT) right->setValue((uint16_t) 0, (uint16_t) 0, (uint16_t) 0);
        }
      }
      else {
        if (likely(top != nullptr)) top->setValue((uint16_t) 0xFFFF);
        if (likely(left != nullptr && rDirection != RotationDirection::LEFT)) left->setValue((uint16_t) 0xFFFF);
        if (likely(right != nullptr && rDirection != RotationDirection::RIGHT)) right->setValue((uint16_t) 0xFFFF);
      }
    }

    inline void __attribute__((__always_inline__)) breakLights() noexcept {
      if (likely(top != nullptr)) top->setValue((uint16_t) 0xFFFF, (uint16_t) 0, (uint16_t) 0);
      if (likely(left != nullptr)) left->setValue((uint16_t) 0xFFFF, (uint16_t) 0, (uint16_t) 0);
      if (likely(right != nullptr)) right->setValue((uint16_t) 0xFFFF, (uint16_t) 0, (uint16_t) 0);      
    }

    inline void __attribute__((__always_inline__)) setRotation(RotationDirection direction) noexcept {
      if (direction != rDirection) {
        
        rDirection = direction;
      }
    }

  private:
 
    Led* const top;
    Led* const left;
    Led* const right;

    volatile bool nigthLigth = true;
    volatile bool direction = true;
    volatile RotationDirection rDirection = RotationDirection::NONE;
};

#endif
