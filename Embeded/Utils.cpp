#include <utils/Utils.h>
#include <Arduino.h>

#define SYNC_THRESHOLD_MS (60 * 1000)

uint64_t IRAM_ATTR currentTimeMillis(uint32_t timeMs) noexcept {
    static volatile uint32_t prevMs = 0;
    static volatile uint64_t currentTimeMs;

    const uint32_t diff = timeMs - prevMs;

    if (unlikely(diff >= SYNC_THRESHOLD_MS || prevMs == 0)) {
        struct timeval time;
        gettimeofday(&time, NULL);
        prevMs = unlikely(time.tv_sec < ZERO_TIME_THRESHOLD_SEC) ? 0 : timeMs;
        return currentTimeMs = (uint64_t) (multBy1000(time.tv_sec) + divBy1000(time.tv_usec));
    }

    return currentTimeMs + diff;
}
