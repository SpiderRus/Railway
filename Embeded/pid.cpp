#include "esp_attr.h"
#include "esp_compiler.h"
#include "utils/Utils.h"
#include <pid\Pid.h>
#include <logger\logger.h>

static const char TAG[] PROGMEM = "PID";

float IRAM_ATTR PID::calc(const float current, const uint32_t currentMs) noexcept {
    const float error = target - current;

    float result = error * Kp; // proportional

    if (likely(prevMillis > 0)) {
        const float deltaSec = (currentMs - prevMillis) / 1000.0f;

        if (likely(deltaSec > 0.0f)) {
            const float delta = prev - current;

            result += delta * Kd / deltaSec; // differential

            integral = constraint(integral + error * Ki * deltaSec); // integral
            result += integral;
        }
    }

    prevMillis = currentMs;
    prev = current;

    return constraint(result);
}

PIDTask::PIDTask(PID& pid, PidSourceCallbackFunc sourceFunc, void *sourceArgs, PidDestinationCallbackFunc destinationFunc, void *destinationArgs,
                                    uint32_t pidStepMs, float minDiff) noexcept
    : QueueWorker<float>(1, "PID", NORMAL_TASK_PRIORITY),
        pid(pid),
        sourceFunc(sourceFunc),
        sourceArgs(sourceArgs),
        destinationFunc(destinationFunc),
        destinationArgs(destinationArgs),
        pidStepMs(pidStepMs), minDiff(minDiff), target(0)  {}

void IRAM_ATTR PIDTask::processQueueItem(float& value) noexcept {
    if (std::abs(value - target) >= minDiff) {
        do {
          rt:
            const bool isZero = std::abs(value) < minDiff;
            const float signValue = signNonZero(value);

            if (isZero || signValue != signNonZero(target)) { // stop
                pid.clear();
                const float safeTarget = target;
                setDestination(target = 0.0f);

                const uint32_t now = currentMillis();
                while (std::abs(getSource()) > minDiff) { // wait stop
                    if (unlikely(receiveQueueItem(value, pidStepMs) && std::abs(value - safeTarget) >= minDiff))
                        goto rt;

                    if (unlikely(currentMillis() - now > 10000)) {
                        SP_LOGW(TAG, "Timeout while wait stop.");
                        return;
                    }
                }

                if (unlikely(isZero))
                    return;
            }

            pid.setTarget(std::abs(target = value));
            do {
                setDestination(signValue * pid.calc(std::abs(getSource())));

                if (unlikely(receiveQueueItem(value, pidStepMs) && std::abs(value - target) >= minDiff))
                    break;
            } while (true);
        } while (true);
    }
}
