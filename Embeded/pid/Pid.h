#ifndef __PID_H
#define __PID_H

#include <Arduino.h>
#include <utils\Utils.h>
#include <utils\QueueWorker.h>

class PID {
  private:
    inline float __attribute__((__always_inline__)) constraint(const float value) const noexcept {
        return std::max(min, std::min(value, max));
    }

  public:
    PID(float Kp, float Kd, float Ki, float min, float max) noexcept :
        Kp(Kp), Kd(Kd), Ki(Ki), min(min), max(max),
        prev(0.0f), integral(0.0f), prevMillis(0) {}

    inline void __attribute__((__always_inline__)) setTarget(float value) noexcept {
        target = value;
    }

    inline void __attribute__((__always_inline__)) clear() noexcept {
        prev = 0.0f;
        integral = 0.0f;
        prevMillis = 0;
    }

    float IRAM_ATTR calc(const float current, const uint32_t currentMs) noexcept;

    inline float __attribute__((__always_inline__)) calc(const float current) noexcept {
        return calc(current, currentMillis());
    }

  private:
    const float Kp, Kd, Ki, min, max;
    float prev, integral, target;
    uint32_t prevMillis;
};

typedef std::function<float(void *args)> PidSourceCallbackFunc;
typedef std::function<void(void *args, float)> PidDestinationCallbackFunc;

class PIDTask : public QueueWorker<float> {
  private:
    inline float __attribute__((__always_inline__)) getSource() noexcept {
        return sourceFunc(sourceArgs);
    }

    inline void __attribute__((__always_inline__)) setDestination(float value) noexcept {
        destinationFunc(destinationArgs, value);
    }

  public:
    PIDTask(PID &pid, PidSourceCallbackFunc sourceFunc, void *sourceArgs, PidDestinationCallbackFunc destinationFunc, void *destinationArgs,
            uint32_t pidStepMs = 100, float minDiff = 1.0f) noexcept;

    inline void __attribute__((__always_inline__)) setTarget(float value) noexcept {
        overwriteQueueItem(value);
    }

  protected:
    void IRAM_ATTR processQueueItem(float& value) noexcept override final;

  private:
    PID &pid;
    PidSourceCallbackFunc sourceFunc;
    void *sourceArgs;
    PidDestinationCallbackFunc destinationFunc;
    void *destinationArgs;

    const float minDiff;
    const uint32_t pidStepMs;
    float target;
};

#endif // __PID_H