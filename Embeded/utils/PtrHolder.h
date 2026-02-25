#ifndef MHOLDER_H
#define MHOLDER_H

#include <cstddef>
#include <utils/Utils.h>

template <typename T> class PtrHolder {
private:
    inline void __attribute__((__always_inline__)) tryRelease() noexcept {
        if (unlikely(pointer != nullptr && --(pointer->_mem_ref_counter) == 0)) {
            delete pointer;
            pointer = nullptr;
        }
    }

public:
    inline __attribute__((__always_inline__)) PtrHolder() noexcept : pointer(nullptr) {
    }

    inline __attribute__((__always_inline__)) explicit PtrHolder(T *p) noexcept : pointer(p) {
        if (likely(pointer != nullptr))
            (pointer->_mem_ref_counter)++;
    }

    inline __attribute__((__always_inline__)) explicit PtrHolder(T *p, unsigned int value) noexcept : pointer(p) {
        if (likely(value && pointer != nullptr))
            pointer->_mem_ref_counter += value;
    }

    inline __attribute__((__always_inline__)) PtrHolder(const PtrHolder<T>& from) noexcept : pointer(from.pointer) {
        if (likely(pointer != nullptr))
            (pointer->_mem_ref_counter)++;
    }

    inline __attribute__((__always_inline__)) PtrHolder(PtrHolder<T>&& from) noexcept {
        pointer = from.pointer;
        from.pointer = nullptr;
    }

    inline __attribute__((__always_inline__)) ~PtrHolder() noexcept {
        tryRelease();
    }

    inline PtrHolder<T>& __attribute__((__always_inline__)) operator=(const PtrHolder<T>& from) noexcept {
        tryRelease();

        if (likely((pointer = from.pointer) != nullptr))
            (pointer->_mem_ref_counter)++;

        return *this;
    }

    inline PtrHolder<T>& __attribute__((__always_inline__)) operator=(PtrHolder<T>&& from) noexcept {
        T* const tmp = pointer;

        pointer = from.pointer;
        from.pointer = tmp;

        return *this;
    }

    inline T* __attribute__((__always_inline__)) ptr() const noexcept {
        return pointer;
    }

    inline int __attribute__((__always_inline__)) refCount() const noexcept {
        return unlikely(pointer == nullptr) ? -1 : (int) pointer->_mem_ref_counter;
    }

    inline bool __attribute__((__always_inline__)) isAllocated() const noexcept {
        return pointer != nullptr;
    }

    inline void __attribute__((__always_inline__)) accure(unsigned int value) noexcept {
        if (likely(pointer != nullptr && value))
            pointer->_mem_ref_counter += value;
    }

    inline void __attribute__((__always_inline__)) release() noexcept {
        tryRelease();
    }

    inline size_t __attribute__((__always_inline__)) store(void *buffer) noexcept {
        if (likely(buffer != nullptr)) {
            *((T **) buffer) = pointer;

            if (likely(pointer != nullptr))
                (pointer->_mem_ref_counter)++;
        }

        return sizeof(pointer);
    }

    static inline PtrHolder<T> __attribute__((__always_inline__)) restore(void *buffer) noexcept {
        if (unlikely(buffer == nullptr))
            return PtrHolder<T>();

        T *ptr = *((T**)buffer);
        *((T**)buffer) = nullptr;

        return PtrHolder<T>(ptr, 0);
    }

private:
    T* pointer;
};

#endif //MHOLDER_H
