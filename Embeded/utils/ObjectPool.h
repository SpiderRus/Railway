
#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <atomic>
#include <malloc.h>
#include <utils/Utils.h>

inline void* __attribute__((__always_inline__)) __op_alloc(size_t size) noexcept {
    return aligned_alloc(alignof(void*), size);
}

template <typename T> class ObjectPool {
private:
    void init() noexcept {
        uint32_t offset = 0;

        for (uint16_t i = 0; i < length - 1; i++) {
            typeof(offset) nextOffset = offset + objectSize;

            ((T*)(array + offset))->_pool_next = nextOffset;
            offset = nextOffset;
        }

        ((T*)(array + offset))->_pool_next = 0xFFFFFFFF;
    }

public:
    ObjectPool(uint16_t length) noexcept :
                    length(length), objectSize(sizeof(T)), arrayLength(length * sizeof(T)), array((uint8_t*)__op_alloc(length * sizeof(T))) {
        init();
    }

    ObjectPool(uint16_t objectSize, uint16_t length) noexcept :
                    length(length), objectSize(objectSize), arrayLength(length * objectSize), array((uint8_t*)__op_alloc(length * objectSize)) {
        init();
    }

    inline __attribute__((__always_inline__)) ~ObjectPool() noexcept {
        if (likely(array != nullptr))
            free(array);
    }

    inline T* __attribute__((__always_inline__)) poll() noexcept {
        uint32_t offset, oldKey;
        T* result;

        do {
            oldKey = first;
            offset = oldKey & 0x000FFFFF;

            if (unlikely(offset == 0x000FFFFF))
                return nullptr;

            result = (T*)(array + offset);
        } while (unlikely(!first.compare_exchange_strong(oldKey, result->_pool_next)));

        return result;
    }

    inline void __attribute__((__always_inline__)) release(T* const ptr) noexcept {
        if (likely(ptr != nullptr)) {
            uint32_t offset = (uint32_t) (((uint8_t*)ptr) - array);

            if (likely(offset < arrayLength)) {
                uint32_t newKey = offset | abaProtectCounter.fetch_add(0x00100000);
                uint32_t oldKey;

                do {
                    oldKey = first;
                    ptr->_pool_next = (uint32_t) (oldKey & 0x000FFFFF);
                } while (unlikely(!first.compare_exchange_strong(oldKey, newKey)));
            }
        }
    }

private:
    const uint16_t objectSize, length;
    const uint32_t arrayLength;
    uint8_t* const array;
    std::atomic_uint32_t first = 0;
    std::atomic_uint32_t abaProtectCounter = 0;
};

template <typename T> class PooledObject {
public:
    inline __attribute__((__always_inline__)) PooledObject(ObjectPool<T>& p) noexcept : pool(p), _mem_ref_counter(0) {}

    inline void __attribute__((__always_inline__)) operator delete(void *ptr) noexcept {
        T * const self = (T*)ptr;

        self->pool.release(self);
    }

    friend class PtrHolder<T>;
    friend class ObjectPool<T>;
private:
    std::atomic_uint _mem_ref_counter;
    ObjectPool<T> &pool;
    volatile uint32_t _pool_next;
};

#endif //OBJECTPOOL_H
