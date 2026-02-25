#ifndef __UTILS_H
#define __UTILS_H

#include <esp_compiler.h>
#include <esp_attr.h>
#include <string.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define UDP_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define LOG_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define STATUS_TASK_PRIORITY (tskIDLE_PRIORITY + 6)
#define FASTLED_TASK_PRIORITY (tskIDLE_PRIORITY + 8)
#define NORMAL_TASK_PRIORITY (tskIDLE_PRIORITY + 10)

#define ZERO_TIME_THRESHOLD_SEC (86400 * 365 * 50)

template<typename T>
inline T __attribute__((__always_inline__)) sign(const T value) noexcept {
    return value > (T)0 ? (T)1 : (value < (T)0 ? (T)-1 : (T)0);
}

template<typename T>
inline T __attribute__((__always_inline__)) signNonZero(const T value) noexcept {
    return value >= (T)0 ? (T)1 : (T)-1;
}

inline float __attribute__((__always_inline__)) multBy1000(const float value) noexcept {
    return value * 1000.0f;
}

inline double __attribute__((__always_inline__)) multBy1000(const double value) noexcept {
    return value * 1000.0;
}

template<typename T>
inline T __attribute__((__always_inline__)) multBy1000(const T value) noexcept {
    return (value << 10) - (value << 4) - (value << 3);
}

template<typename T>
inline T __attribute__((__always_inline__)) divBy1000(const T value) noexcept {
    return value / (T)1000;
}

template<typename T>
inline T __attribute__((__always_inline__)) abs(const T value) noexcept {
    return value >= (T)0 ? value : (-value);
}

inline bool __attribute__((__always_inline__)) equals(const float value1, const float value2, const float threshold) noexcept {
    return std::abs(value2 - value1) < threshold;
}

inline bool __attribute__((__always_inline__)) equals(const float value1, const float value2) noexcept {
    return equals(value1, value2, 0.001f);
}

/*
class BufferWriter {
public:
    inline __attribute__((__always_inline__)) explicit BufferWriter(void *buffer) noexcept : buffer(buffer), offset(0) {}

    inline void __attribute__((__always_inline__)) append(const uint8_t value) noexcept {
        *(uint8_t*)(buffer + offset) = value;
        offset++;
    }

    inline void __attribute__((__always_inline__)) append(const uint16_t value) noexcept {
        *(uint16_t*)(buffer + offset) = value;
        offset += sizeof(uint16_t);
    }

    inline void __attribute__((__always_inline__)) append(const uint32_t value) noexcept {
        *(uint32_t*)(buffer + offset) = value;
        offset += sizeof(uint32_t);
    }

    inline void __attribute__((__always_inline__)) append(const uint64_t value) noexcept {
        *(uint64_t*)(buffer + offset) = value;
        offset += sizeof(uint64_t);
    }

    inline void __attribute__((__always_inline__)) append(const void * const from, const size_t length) noexcept {
        if (likely(length > 0)) {
            append((uint16_t)length);
            memcpy(buffer + offset, from, length);
            offset += length;
        }
    }


    inline size_t __attribute__((__always_inline__)) length() const noexcept { return offset; }
private:
    void* const buffer;
    size_t offset;
};
*/

class MemBuffer {
public:
    inline __attribute__((__always_inline__)) explicit MemBuffer(const void *buffer, size_t size) noexcept : Buffer(buffer), Size(size) {}

    [[nodiscard]] inline size_t __attribute__((__always_inline__)) size() const noexcept {
        return Size;
    }

    [[nodiscard]] inline const void * __attribute__((__always_inline__)) buffer() const noexcept {
        return Buffer;
    }

private:
    const size_t Size;
    const void * const Buffer;
};

class uint8Array {
public:
    inline __attribute__((__always_inline__)) explicit uint8Array(const uint8_t *array, size_t size) noexcept : Array(array), Size(size) {}

    [[nodiscard]] inline size_t __attribute__((__always_inline__)) size() const noexcept {
        return Size;
    }

    [[nodiscard]] inline const uint8_t * __attribute__((__always_inline__)) array() const noexcept {
        return Array;
    }

private:
    const size_t Size;
    const uint8_t * const Array;
};

class uint32Array {
public:
    inline __attribute__((__always_inline__)) explicit uint32Array(const uint32_t *array, size_t size) noexcept : Array(array), Size(size) {}

    [[nodiscard]] inline size_t __attribute__((__always_inline__)) size() const noexcept {
        return Size;
    }

    [[nodiscard]] inline const uint32_t * __attribute__((__always_inline__)) array() const noexcept {
        return Array;
    }

private:
    const size_t Size;
    const uint32_t * const Array;
};

class BinarySerializer {
private:
    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, uint32Array &from, const size_t maxLen) noexcept {
        const ptrdiff_t arrayLength = from.size() * sizeof(uint32_t);
        const ptrdiff_t length = arrayLength + sizeof(uint16_t);
        if (unlikely(length > maxLen))
            return -1;

        *(uint16_t *) buffer = (uint16_t) from.size();
        memcpy((uint8_t *) buffer + sizeof(uint16_t), from.array(), arrayLength);

        return length;
    }

    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, uint8Array &from, const size_t maxLen) noexcept {
        const ptrdiff_t arrayLength = from.size();
        const ptrdiff_t length = arrayLength + sizeof(uint16_t);
        if (unlikely(length > maxLen))
            return -1;

        *(uint16_t *) buffer = (uint16_t) from.size();
        memcpy((uint8_t *) buffer + sizeof(uint16_t), from.array(), arrayLength);

        return length;
    }

    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, MemBuffer &from, const size_t maxLen) noexcept {
        const ptrdiff_t length = from.size() + sizeof(uint16_t);
        if (unlikely(length > maxLen))
            return -1;

        *(uint16_t *) buffer = (uint16_t) from.size();
        memcpy((uint8_t *) buffer + sizeof(uint16_t), from.buffer(), from.size());

        return length;
    }

    static inline ptrdiff_t __attribute__((__always_inline__)) vstrncpy(char *dst, const char *src, const size_t maxLen) noexcept {
        ptrdiff_t result;

        for (result = 0; *src; result++) {
            if (unlikely(result == maxLen))
                return -1;

            *dst++ = *src++;
        }

        return result;
    }

    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, const char *field, const size_t maxLen) noexcept {
        ptrdiff_t length;

        if (unlikely(maxLen < 2 || (length = vstrncpy((char *) buffer + sizeof(uint16_t), field, maxLen - sizeof(uint16_t))) < 0))
            return -1;

        *(uint16_t *) buffer = (uint16_t) length;

        return length + sizeof(uint16_t);
    }

    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, char *field, const size_t maxLen) noexcept {
        return writer(buffer, (const char *) field, maxLen);
    }

    template<typename Tp>
    static inline ptrdiff_t __attribute__((__always_inline__)) writer(void * const buffer, Tp field, const size_t maxLen) noexcept {
        if (unlikely(sizeof(Tp) > maxLen))
            return -1;

        *(Tp *) buffer = field;

        return sizeof(Tp);
    }

    static inline ptrdiff_t serialize(void * const buffer, const size_t maxLen) noexcept {
        return 0;
    }

public:
    template<typename Tp, typename... Types>
    static inline ptrdiff_t __attribute__((__always_inline__)) serialize(void * const buffer, const size_t maxLen, Tp field, Types... tail) noexcept {
        const ptrdiff_t length = writer(buffer, field, maxLen);

        if (unlikely(length < 0))
            return length;

        const ptrdiff_t subLength = serialize((uint8_t *) buffer + length, maxLen - length, tail...);

        return unlikely(subLength < 0) ? subLength : (length + subLength);
    }
};

inline uint32_t __attribute__((__always_inline__)) currentMillis(void) noexcept {
    return (uint32_t) (divBy1000(esp_timer_get_time()));
}

extern uint64_t IRAM_ATTR currentTimeMillis(uint32_t current) noexcept;

inline uint64_t __attribute__((__always_inline__)) currentTimeMillis(void) noexcept {
    return currentTimeMillis(currentMillis());
}

inline char* __attribute__((__always_inline__)) strncpyz(char * const dst, const char * const src, const size_t maxLen) noexcept {
    const size_t zeroIndex = maxLen - 1;

    strncpy(dst, src, zeroIndex);
    dst[zeroIndex] = '\0';

    return dst;
}

inline void __attribute__((__always_inline__)) yieldTask() noexcept {
    const auto currentPriority = uxTaskPriorityGet(NULL);

    vTaskPrioritySet(NULL, tskIDLE_PRIORITY);
    taskYIELD();
    vTaskPrioritySet(NULL, currentPriority);
}

template<typename T>
inline void __attribute__((__always_inline__)) taskDelay(T delayMs) noexcept {
    if (unlikely(delayMs > (T)0))
        vTaskDelay(pdMS_TO_TICKS(delayMs));
}

template<typename T>
inline T* __attribute__((__always_inline__)) newArray(const size_t length) noexcept {
    return (T*) new uint8_t[length * sizeof(T)];
}

template<typename T, int floatBits>
class FloatBase {
private:
    static inline FloatBase<T, floatBits> __attribute__((__always_inline__)) instantiate(T value) noexcept {
        FloatBase<T, floatBits> result;
        result.value = value;
        return result;
    }

public:
    static inline FloatBase<T, floatBits> __attribute__((__always_inline__)) max(const FloatBase<T, floatBits>& v1, const FloatBase<T, floatBits>& v2) noexcept {
        return v1.value > v2.value ? v1 : v2;
    }

    static inline FloatBase<T, floatBits> __attribute__((__always_inline__)) min(const FloatBase<T, floatBits>& v1, const FloatBase<T, floatBits>& v2) noexcept {
        return v1.value < v2.value ? v1 : v2;
    }

    inline __attribute__((__always_inline__)) FloatBase() noexcept = default;

    inline __attribute__((__always_inline__)) FloatBase(const int value) noexcept : value((T) (value << floatBits)) {}

    inline __attribute__((__always_inline__)) FloatBase(const float value) noexcept : value((T) (value * multiplier)) {}

    inline __attribute__((__always_inline__)) FloatBase(const FloatBase<T, floatBits>& from) noexcept : value(from.value) {}

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator=(const FloatBase<T, floatBits>& from) noexcept {
        value = from.value;
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator=(const int val) noexcept {
        value = val << floatBits;
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator=(const float val) noexcept {
        value = (T) (val * multiplier);
        return *this;
    }

    inline bool __attribute__((__always_inline__)) operator<(const FloatBase<T, floatBits>& other) noexcept {
        return value < other.value;
    }

    inline bool __attribute__((__always_inline__)) operator<=(const FloatBase<T, floatBits>& other) noexcept {
        return value <= other.value;
    }

    inline bool __attribute__((__always_inline__)) operator>(const FloatBase<T, floatBits>& from) noexcept {
        return value > from.value;
    }

    inline bool __attribute__((__always_inline__)) operator>=(const FloatBase<T, floatBits>& from) noexcept {
        return value >= from.value;
    }

    inline bool __attribute__((__always_inline__)) operator==(const FloatBase<T, floatBits>& from) noexcept {
        return value == from.value;
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator*(const FloatBase<T, floatBits>& other) const noexcept {
        return instantiate((T) (value * float(other)));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator*(const int val) const noexcept {
        return instantiate((T) (value * val));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator*(const unsigned int val) const noexcept {
        return instantiate((T) (value * val));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator/(const FloatBase<T, floatBits>& other) const noexcept {
        return instantiate((T) (value / float(other)));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator/(const int val) const noexcept {
        return instantiate((T) (value / val));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator/(const unsigned int val) const noexcept {
        return instantiate((T) (value / val));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator+(const FloatBase<T, floatBits>& other) const noexcept {
        return instantiate(value + other.value);
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator+(const float other) const noexcept {
        return instantiate(value + (T) (other * multiplier));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator+(const int other) const noexcept {
        return instantiate(value + (T) (other << floatBits));
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator+(const unsigned int other) const noexcept {
        return instantiate(value + (T) (other << floatBits));
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator+=(const FloatBase<T, floatBits> other) noexcept {
        value += other.value;
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator+=(const float other) noexcept {
        value += (T) (other * multiplier);
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator+=(const int other) noexcept {
        value += (T) (other << floatBits);
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator+=(const unsigned int other) noexcept {
        value += (T) (other << floatBits);
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator-=(const FloatBase<T, floatBits> other) noexcept {
        value -= other.value;
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator-=(const float other) noexcept {
        value -= (T) (other * multiplier);
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator-=(const int other) noexcept {
        value -= (T) (other << floatBits);
        return *this;
    }

    inline FloatBase<T, floatBits>& __attribute__((__always_inline__)) operator-=(const unsigned int other) noexcept {
        value -= (T) (other << floatBits);
        return *this;
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator-(const FloatBase<T, floatBits>& other) const noexcept {
        return instantiate(value - other.value);
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) operator-(const int other) const noexcept {
        return instantiate(value - other);
    }

    inline __attribute__((__always_inline__)) explicit operator float() const noexcept {
        return ((float) value) / multiplier;
    }

    inline __attribute__((__always_inline__)) explicit operator int() const noexcept {
        return (int) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator unsigned int() const noexcept {
        return (unsigned int) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator int8_t() const noexcept {
        return (int8_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator uint8_t() const noexcept {
        return (uint8_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator int16_t() const noexcept {
        return (int16_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator uint16_t() const noexcept {
        return (uint16_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator int32_t() const noexcept {
        return (int32_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator uint32_t() const noexcept {
        return (uint32_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator int64_t() const noexcept {
        return (int64_t) (value >> floatBits);
    }

    inline __attribute__((__always_inline__)) explicit operator uint64_t() const noexcept {
        return (uint64_t) (value >> floatBits);
    }

    inline FloatBase<T, floatBits> __attribute__((__always_inline__)) abs() const noexcept {
        return instantiate(std::abs(value));
    }

    inline int __attribute__((__always_inline__)) round() const noexcept {
        const int p = value >> (floatBits - 1);

        return (p >> 1) + (p & 1);
    }

    inline bool __attribute__((__always_inline__)) isNegative() const noexcept {
        return value & hiBit;
    }
private:
    static constexpr unsigned int hiBit = 1 << (8 * sizeof(T) - 1);
    static constexpr unsigned int multiplier = 1 << floatBits;

    T value;
};

using Float16 = FloatBase<int16_t, 8>;

using Float32 = FloatBase<int32_t, 16>;

using Float64 = FloatBase<int64_t, 24>;

#endif // __UTILS_H
