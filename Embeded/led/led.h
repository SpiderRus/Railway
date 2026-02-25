#ifndef __LED_H
#define __LED_H

#include <config/config.h>

#ifdef USE_LEDS

#include <algorithm>
#include "esp32-hal.h"
#include <Arduino.h>
#include <esp32-hal-ledc.h>
#include <FastLED.h>
#include <atomic>
#include <utils/Utils.h>
#include <utils/QueueWorker.h>

class Led {
public:
    inline void __attribute__((__always_inline__)) setValue(float value) noexcept {
        setValue(convert(value));
    }

    inline void __attribute__((__always_inline__)) on() noexcept {
        setValue((uint8_t)0xFF);
    }

    inline void __attribute__((__always_inline__)) off() noexcept {
        setValue((uint8_t)0);
    }

    inline void __attribute__((__always_inline__)) negative() noexcept {
        setValue((uint8_t)(getValue() == 0 ? 0xFF : 0));
    }

    virtual void setValue(uint8_t val) noexcept;

    virtual uint8_t getValue() noexcept;

    virtual bool begin() noexcept {
        return true;
    }

protected:
    inline uint8_t __attribute__((__always_inline__)) convert(float value) noexcept {
      return value > 0 ? (value < 1.0 ? (uint8_t) (0xFF * value) : 0xFF) : 0;
    }
};

class PwmLed : public Led {
public:
    PwmLed(uint8_t Pin, boolean reversed = false, uint32_t freq = 10000) noexcept
                : pin(pin), reversed(reversed), frequency(freq), value(0) {}

    uint8_t IRAM_ATTR getValue() noexcept override final;

    void IRAM_ATTR setValue(uint8_t val) noexcept override final;
    
    bool begin() noexcept override final;

private:
    const uint8_t pin;
    const boolean reversed;
    const uint32_t frequency;

    uint8_t value;
};

class FastLedTask {
public:
    FastLedTask(uint8_t pin, CRGB *values, size_t size) noexcept;

    inline void __attribute__((__always_inline__)) update() noexcept {
        if (unlikely(updNum++ == 0))
            xQueueOverwrite(queue, &fake);
    }
    
private:
    void IRAM_ATTR task() noexcept;

    static void taskStatic(void *args) noexcept;

private:
    static uint32_t fake;

    CRGB* const values;
    const size_t size;
    CLEDController& controller;
    std::atomic_uint32_t updNum;
    volatile uint32_t prevUpdateMs;

    TaskHandle_t taskHandle;
    QueueHandle_t queue;
};

class FlLed : public Led {
public:
    FlLed(volatile uint8_t& ptr, FastLedTask& task) noexcept : ptr(ptr), task(task) {}

    uint8_t IRAM_ATTR getValue() noexcept override final;

    void IRAM_ATTR setValue(uint8_t val) noexcept override final;
    
private:
    volatile uint8_t& ptr;
    FastLedTask& task;
};

typedef struct _LED_TASK_INFO_PARAMS {
    Led *led;
    uint8_t target;
    uint16_t millis;
} LED_TASK_INFO_PARAMS;

using Float32_10 = FloatBase<int32_t, 22>;

typedef struct _LED_TASK_INFO {
    Led *led;
    uint32_t nextMs;
    Float32_10 current, step, target;
} LED_TASK_INFO;

#define LED_TASK_MIN_UPDATE_PERIOD 30

class LedTask : public QueueWorker<LED_TASK_INFO_PARAMS> {
  public:
    LedTask() noexcept : QueueWorker<LED_TASK_INFO_PARAMS>(MAX_LEDS_IN_TASK, "LED_TASK", FASTLED_TASK_PRIORITY + 1), numTasks(0) {}

    inline void __attribute__((__always_inline__)) process(Led* led, uint8_t value, uint16_t millis) noexcept {
        if (likely(millis < LED_TASK_MIN_UPDATE_PERIOD))
            led->setValue(value);
        else {
            LED_TASK_INFO_PARAMS params = { led, value, millis };
            sendQueueItem(params);
        }
    }

  private:
    void IRAM_ATTR addTask(LED_TASK_INFO_PARAMS &params, uint32_t now) noexcept;

    inline void __attribute__((__always_inline__)) removeTask(size_t index) noexcept {
        if (index < --numTasks)
            memcpy(&tasks[index], &tasks[numTasks], sizeof(LED_TASK_INFO));
    }

  protected:
    void IRAM_ATTR processQueueItem(LED_TASK_INFO_PARAMS &params) noexcept override final;

  private:
    LED_TASK_INFO tasks[MAX_LEDS_IN_TASK];
    size_t numTasks;
};

class SmoothlyLed : public Led {
public:
    SmoothlyLed(Led &led, LedTask& task, const uint16_t *millisPtr = &defMillisValue) noexcept : led(led), task(task), millisPtr(millisPtr) {}

    inline void __attribute__((__always_inline__)) setMillisPtr(const uint16_t *ptr) noexcept {
        millisPtr = ptr;
    }

    uint8_t IRAM_ATTR getValue() noexcept override final {
        return led.getValue();
    }

    void IRAM_ATTR setValue(uint8_t value) noexcept override final {
        task.process(&led, value, *millisPtr);
    }

    bool begin() noexcept override final {
        return led.begin();
    }
  
private:
    static const uint16_t defMillisValue;

    Led &led;
    LedTask &task;
    const uint16_t *millisPtr;
};

#endif

#endif // __LED_H
