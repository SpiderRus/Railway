#ifndef INC_SWITCH_H
#define INC_SWITCH_H

#include <config/config.h>

#ifdef USE_SWITCH

#include <utils/Utils.h>
#include <utils/QueueWorker.h>

class Switch : protected QueueWorker<bool> {
  protected:
    Switch(const char *name, uint16_t smoothMs, uint16_t smoothSteps, uint16_t maxValue, uint16_t stabValue, uint8_t res, uint32_t freq) noexcept;

  public:
    inline bool __attribute__((__always_inline__)) getValue(void) const noexcept { return currentValue; }

    inline void __attribute__((__always_inline__)) setValue(bool value) noexcept {
        overwriteQueueItem(value);
    }

    virtual void begin(void) noexcept = 0;

  protected:
    const uint8_t resolution;

    const uint16_t maxValue, stabValue, smoothMs, smoothSteps;
    const uint32_t frequency, pause, high;
    const float step;

    volatile bool currentValue;
};

class SimpleSwitch : public Switch {
  public:
    SimpleSwitch(bool releaseState, uint8_t pin, uint16_t smoothMs, uint16_t smoothSteps, 
            uint16_t maxValue, uint16_t stabValue, uint8_t res = 10, uint32_t freq = 30000) noexcept;

    void begin(void) noexcept override final;

  protected:
    void IRAM_ATTR processQueueItem(bool& data) noexcept override final;

  private:
    const uint8_t pin;
    const bool releaseState;
};

class BidirectionalSwitch : public Switch {
  public:
    BidirectionalSwitch(uint8_t plusPwmPin, uint8_t minusPwmPin, uint16_t smoothMs, uint16_t smoothSteps, 
            uint16_t maxValue, uint16_t stabValue, uint8_t res = 10, uint32_t freq = 30000) noexcept;

    void begin(void) noexcept override final;

  protected:
    void IRAM_ATTR processQueueItem(bool& data) noexcept override final;

  private:
    const uint8_t plusPwmPin, minusPwmPin;
};

#endif

#endif // INC_SWITCH_H
