#include <led/led.h>

#ifdef USE_LEDS

#include "esp32-hal-gpio.h"
#include <esp32-hal.h>
#include <logger/logger.h>
#include "utils/Utils.h"

#define UPDATE_PERIOD_MS 30

static const char TAG[] PROGMEM = "LED";

const uint16_t SmoothlyLed::defMillisValue(250);

static CLEDController& getController(uint8_t pin, CRGB *values, size_t size) noexcept {
    switch (pin) {
        case 1: return FastLED.addLeds<WS2811, 1, RGB>(values, size);
        case 2: return FastLED.addLeds<WS2811, 2, RGB>(values, size);
        case 3: return FastLED.addLeds<WS2811, 3, RGB>(values, size);
        case 4: return FastLED.addLeds<WS2811, 4, RGB>(values, size);
        case 5: return FastLED.addLeds<WS2811, 5, RGB>(values, size);
        // case 6: return FastLED.addLeds<WS2811, 6, RGB>(values, size);
        // case 7: return FastLED.addLeds<WS2811, 7, RGB>(values, size);
        // case 8: return FastLED.addLeds<WS2811, 8, RGB>(values, size);
        // case 9: return FastLED.addLeds<WS2811, 9, RGB>(values, size);
        // case 10: return FastLED.addLeds<WS2811, 10, RGB>(values, size);
        // case 11: return FastLED.addLeds<WS2811, 11, RGB>(values, size);
        // case 12: return FastLED.addLeds<WS2811, 12, RGB>(values, size);
        // case 13: return FastLED.addLeds<WS2811, 13, RGB>(values, size);
        // case 14: return FastLED.addLeds<WS2811, 14, RGB>(values, size);
        // case 15: return FastLED.addLeds<WS2811, 15, RGB>(values, size);
        // case 16: return FastLED.addLeds<WS2811, 16, RGB>(values, size);
        // case 17: return FastLED.addLeds<WS2811, 17, RGB>(values, size);
        case 18: return FastLED.addLeds<WS2811, 18, RGB>(values, size);
        case 19: return FastLED.addLeds<WS2811, 19, RGB>(values, size);
    }

    return FastLED.addLeds<WS2811, 0, RGB>(values, size);
}

uint8_t IRAM_ATTR PwmLed::getValue() noexcept {
    return value;
}

void IRAM_ATTR PwmLed::setValue(uint8_t val) noexcept {
    if (likely(val != value)) {
        ledcWrite(pin, unlikely(reversed) ? 0xFF - val : val);
        value = val;
    }
}
    
bool PwmLed::begin() noexcept {
    pinMode(pin, OUTPUT);
    ledcAttach(pin, frequency, 8);
    ledcWrite(pin, unlikely(reversed) ? 0xFF : 0);

    return true;
}

uint8_t IRAM_ATTR FlLed::getValue() noexcept {
    return ptr;
}

void IRAM_ATTR FlLed::setValue(uint8_t val) noexcept {
    if (likely(val != ptr)) {
        ptr = val;
        task.update();
    }
}

uint32_t FastLedTask::fake = 0;

FastLedTask::FastLedTask(uint8_t pin, CRGB *values, size_t size) noexcept :
                        values(values), size(size), updNum(0), controller(getController(pin, values, size)), 
                        queue(xQueueCreate(1, sizeof(uint32_t))), prevUpdateMs(0) {
    controller.showLeds(255);
    xTaskCreate(taskStatic, "FASTLED_TASK", 4096, this, FASTLED_TASK_PRIORITY, &taskHandle);
    update();
}

void FastLedTask::taskStatic(void *args) noexcept {
    ((FastLedTask*)args)->task(); 
}

void IRAM_ATTR FastLedTask::task() noexcept {    
    do {
        if (xQueueReceive(queue, &fake, portMAX_DELAY)) {
            uint32_t updates;

            do {
              rt:  
                updates = updNum;

                if (likely(updates > 0)) {
                    const uint32_t now = currentMillis();
                    const uint32_t timeDiff = now - prevUpdateMs;

                    if (unlikely(timeDiff < UPDATE_PERIOD_MS)) {
                        taskDelay(UPDATE_PERIOD_MS - timeDiff);
                        goto rt;
                    }

                    prevUpdateMs = now;
                    controller.showLeds(255);
                } 
            } while (updates > 0 && (updNum -= updates) > 0);
        }
    } while (true);
}

void IRAM_ATTR LedTask::addTask(LED_TASK_INFO_PARAMS &params, uint32_t now) noexcept {
    LED_TASK_INFO *task;

    for (size_t i = 0; i < numTasks; i++) {
        task = &tasks[i];

        if (unlikely(task->led == params.led))
            goto calc;
    }

    if (unlikely(numTasks >= MAX_LEDS_IN_TASK))
        return;

    task = tasks + numTasks++;
    task->led = params.led;

calc:
    task->target = params.target;
    const uint8_t current = params.led->getValue();

    task->current = current;
    task->step = Float32_10(LED_TASK_MIN_UPDATE_PERIOD * int(params.target - current)) / params.millis;
    task->nextMs = now + LED_TASK_MIN_UPDATE_PERIOD;
}

void IRAM_ATTR LedTask::processQueueItem(LED_TASK_INFO_PARAMS &params) noexcept {
    static const Float32_10 ONE(1);

    uint32_t now = currentMillis();
    
    addTask(params, now);
    while (receiveQueueItem(params, 0))
        addTask(params, now);

    do {
        int32_t minMs = 0x7FFFFFFF;

        for (int i = ((int) numTasks) - 1; i >= 0; i--) {
            LED_TASK_INFO &task = tasks[i];

            if (unlikely(task.nextMs <= now)) {
                Float32_10 current = task.current + task.step;
                current = task.step.isNegative() ? Float32_10::max(current, task.target) : Float32_10::min(current, task.target);

                task.led->setValue((uint8_t) current);

                if (unlikely((task.target - current).abs() < ONE)) {
                    removeTask(i);
                    continue;
                }

                task.current = current;
                task.nextMs += LED_TASK_MIN_UPDATE_PERIOD;
            }

            minMs = std::min(minMs, (int32_t) task.nextMs);
        }
        
        if (unlikely(numTasks == 0))
            break;

        const int32_t pauseMs = minMs - currentMillis();

        if (unlikely(pauseMs <= 0))
            yieldTask();

        if (receiveQueueItem(params, pauseMs)) {
            now = currentMillis();

            addTask(params, now);
            while (receiveQueueItem(params, 0))
                addTask(params, now);
        }

        now = currentMillis();
    } while (true);
}

#endif
