#ifndef INC_SEMAPHORE_H
#define INC_SEMAPHORE_H

#include <config/config.h>

#ifdef USE_SEMAPHORE

#include "esp_compiler.h"
#include <Arduino.h>
#include <led/led.h>
#include <logger/logger.h>

class SColor {
public:
    static const SColor RED;
    static const SColor GREEN;
    static const SColor YELLOW;
    static const SColor BLACK;

    inline __attribute__((__always_inline__)) SColor() noexcept : rgb(0) {}

    inline __attribute__((__always_inline__)) SColor(const uint32_t rgb) noexcept : rgb(rgb) {}

    inline __attribute__((__always_inline__)) SColor(const uint8_t r, const uint8_t g, const uint8_t b) noexcept :
        rgb( ((uint32_t)r) | (((uint32_t)g) << 8) | (((uint32_t)b) << 16) ) {}

    inline __attribute__((__always_inline__)) SColor(const SColor& other) noexcept : rgb(other.rgb) {}

    inline SColor& __attribute__((__always_inline__)) operator=(const SColor& other) noexcept {
        rgb = other.rgb;
        return *this;
    }

    inline bool __attribute__((__always_inline__)) operator==(const SColor& other) const noexcept {
        return rgb == other.rgb;
    }

    inline bool __attribute__((__always_inline__)) operator!=(const SColor& other) const noexcept {
        return rgb != other.rgb;
    }

    inline uint32_t __attribute__((__always_inline__)) getValue() const noexcept {
        return rgb;
    }

    inline uint8_t __attribute__((__always_inline__)) getR() const noexcept {
        return (uint8_t) (rgb & 0xFF);
    }

    inline uint8_t __attribute__((__always_inline__)) getG() const noexcept {
        return (uint8_t) ((rgb >> 8) & 0xFF);
    }

    inline uint8_t __attribute__((__always_inline__)) getB() const noexcept {
        return (uint8_t) ((rgb >> 16) & 0xFF);
    }

private:
    uint32_t rgb;    
};

class Semaphore {
  protected:
    Semaphore() noexcept {}

  public:
    inline SColor __attribute__((__always_inline__)) getValue() noexcept {
        return value;
    }

    inline void __attribute__((__always_inline__)) setValue(const SColor& color) noexcept {
        SColor toSet = color.getR() && color.getG()
                ? SColor::YELLOW
                : (color.getR() ? SColor::RED : (color.getG() ? SColor::GREEN : SColor::BLACK));

        if (likely(toSet != value))
            setValueInternal(value = toSet);
    }

    virtual void begin() noexcept;

  protected:
    virtual void setValueInternal(const SColor& color) noexcept;

  protected:
    SColor value;
};

class SemaphoreBase : public Semaphore {
  public:
    SemaphoreBase(Led &red, Led &green, Led &blue) noexcept : red(red), green(green), blue(blue) {}

    void begin() noexcept override {
        red.begin();
        green.begin();
        blue.begin();
    }

  protected:
    void IRAM_ATTR setValueInternal(const SColor& color) noexcept override;

  protected:
    Led &red;
    Led &green;
    Led &blue;
};

class SemaphoreRGBBase : public Semaphore {
  public:
    SemaphoreRGBBase(Led &green_red, Led &green_green, Led &green_blue,
                    Led &yellow_red, Led &yellow_green, Led &yellow_blue,
                    Led &red_red, Led &red_green, Led &red_blue) noexcept
                    : green_red(green_red), green_green(green_green), green_blue(green_blue),
                        yellow_red(yellow_red), yellow_green(yellow_green), yellow_blue(yellow_blue),
                        red_red(red_red), red_green(red_green), red_blue(red_blue) {}

    void begin() noexcept override {
        green_red.begin();
        green_green.begin();
        green_blue.begin();

        yellow_red.begin();
        yellow_green.begin();
        yellow_blue.begin();

        red_red.begin();
        red_green.begin();
        red_blue.begin();
    }

  protected:
    void IRAM_ATTR setValueInternal(const SColor& color) noexcept override;

  protected:
    Led &green_red;
    Led &green_green;
    Led &green_blue;

    Led &yellow_red;
    Led &yellow_green;
    Led &yellow_blue;

    Led &red_red;
    Led &red_green;
    Led &red_blue;
};

class Semaphores {
  public:
    virtual void begin() noexcept;

    virtual size_t count() noexcept;

    virtual Semaphore& get(const size_t index) noexcept;
};

template<size_t NUM>
class FlSemaphores : public Semaphores {
  private:
    inline SemaphoreBase& __attribute__((__always_inline__)) semaphore(const size_t index) noexcept {
        return ((SemaphoreBase*)semaphoresBuffer)[index];
    }

    Led& createLed(size_t index) noexcept {
        return *new (((SmoothlyLed*)smoothLedsBuffer) + index) SmoothlyLed(
                    *new (((FlLed*)ledsBuffer) + index) FlLed(ledColors[index / 3].raw[index % 3], flTask), ledTask);
    }

  public:
    FlSemaphores(uint8_t pin) noexcept : flTask(pin, ledColors, NUM) {        
        memset(ledColors, 0, NUM * sizeof(CRGB));

        for (size_t i = 0, index = 0; i < NUM; i++, index += 3)
            new (((SemaphoreBase*)semaphoresBuffer) + i) SemaphoreBase(createLed(index), createLed(index + 1), createLed(index + 2));
    }

    void begin() noexcept override final {
        for (size_t i = 0; i < NUM; i++)
            semaphore(i).begin();
    }

    size_t IRAM_ATTR count() noexcept override final {
        return NUM;
    }

    Semaphore& IRAM_ATTR get(const size_t index) noexcept override final {
        return semaphore(index);
    }

  private:
    CRGB ledColors[NUM];
    FastLedTask flTask;
    LedTask ledTask;

    uint8_t semaphoresBuffer[NUM * sizeof(SemaphoreBase)];
    uint8_t ledsBuffer[3 * NUM * sizeof(FlLed)];
    uint8_t smoothLedsBuffer[3 * NUM * sizeof(SmoothlyLed)];
};

class FlRGBSemaphores : public Semaphores {
  private:
    Led& createLed(size_t index) noexcept {
        return *new (smoothlyLeds + index) SmoothlyLed(
                        *new (flLeds + index) FlLed(ledColors[index / 3].raw[index % 3], flTask),
                    ledTask);
    }

  public:
    FlRGBSemaphores(const uint8_t pin, const size_t num) noexcept :
                NUM(num),
                ledColors(newArray<CRGB>(3 * num)),
                flLeds(newArray<FlLed>(9 * num)),
                smoothlyLeds(newArray<SmoothlyLed>(9 * num)),
                semaphores(newArray<SemaphoreRGBBase>(num)),
                flTask(pin, ledColors, 3 * num) {
        memset(ledColors, 0, 3 * num * sizeof(CRGB));

        for (size_t i = 0, index = 0; i < num; i++, index += 9) {
            new (semaphores + i) SemaphoreRGBBase(
                    createLed(index), createLed(index + 1), createLed(index + 2),
                    createLed(index + 3), createLed(index + 4), createLed(index + 5),
                    createLed(index + 6), createLed(index + 7), createLed(index + 8)
                );
        }
    }

    ~FlRGBSemaphores() noexcept {
        delete ledColors;
        delete flLeds;
        delete smoothlyLeds;
        delete semaphores;
    }

    void begin() noexcept override final {
        for (size_t i = 0; i < NUM; i++)
            semaphores[i].begin();
    }

    size_t IRAM_ATTR count() noexcept override final {
        return NUM;
    }

    Semaphore& IRAM_ATTR get(const size_t index) noexcept override final {
        return semaphores[index];
    }

  private:
    const size_t NUM;
    CRGB* const ledColors;

    FastLedTask flTask;
    LedTask ledTask;

    SemaphoreRGBBase* const semaphores;
    FlLed* const flLeds;
    SmoothlyLed* const smoothlyLeds;
};

#endif

#endif // INC_SEMAPHORE_H
