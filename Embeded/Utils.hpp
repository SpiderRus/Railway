#ifndef INC_UTILS_HPP
#define INC_UTILS_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <atomic>


#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

#define BUFFER_HEADER_SIZE (sizeof(std::atomic_uint) + sizeof(size_t))

#define sign(value) ((value) > -0 ? 1 : -1)

extern String textPlain;
extern String textHtml;
extern String applicationJson;
extern String EMPTY_STR;

inline bool __attribute__((__always_inline__)) start_with(const char *str, const char *with) noexcept {
  char c, c1;
  
  do {
    c = *with++;
    c1 = *str++;
  } while(c && c == c1);

  return c != c1;
}

inline size_t __attribute__((__always_inline__)) s_len(const char *str) noexcept {
  size_t len = 0;

  if (str != NULL)
    while (str[len]) 
      len++;

  return len;
}

inline char* __attribute__((__always_inline__)) copy_str(char *dst, const char *src) noexcept {
  for (; *dst = *src; dst++, src++)
    ;

  return dst;
}

inline size_t __attribute__((__always_inline__)) copy_strn(char *dst, const char *src) noexcept {
  char c;
  size_t count = 0;

  do {
    c = *(src++);
    *(dst++) = c;
    count++;
  } while(c);

  return count;
}

inline uint64_t __attribute__((__always_inline__)) toMillis(struct timeval &time) noexcept {
  return 1000 * ((uint64_t)time.tv_sec) + time.tv_usec / 1000;
}

inline uint64_t __attribute__((__always_inline__)) currentTimeMillis() noexcept {
  struct timeval time;
  gettimeofday(&time, NULL);

  return toMillis(time);
}

class Buffer {
  public:
    inline __attribute__((__always_inline__)) Buffer() noexcept : buffer(NULL) {
    } 

    inline __attribute__((__always_inline__)) Buffer(size_t size, uint value = 1) noexcept : buffer(malloc(size + BUFFER_HEADER_SIZE)) {
      if (buffer) {
        std::atomic_uint * const counter = new (buffer) std::atomic_uint();
        *counter = value;
        *(size_t*)(buffer + sizeof(std::atomic_uint)) = size;
      }
    }

    inline __attribute__((__always_inline__)) Buffer(const Buffer& from) noexcept : buffer(from.buffer) {
      if (buffer) {
        std::atomic_uint * const counter = (std::atomic_uint*) buffer;
        (*counter)++;
        //Serial.printf("Buffer constructor copy %p\n", buffer);
      }
    }

    inline __attribute__((__always_inline__)) Buffer(Buffer&& from) noexcept {
      void *tmp = buffer;

      buffer = from.buffer;
      from.buffer = tmp;
    }

    __attribute__((__always_inline__)) ~Buffer() noexcept {
      if (buffer) {
        std::atomic_uint * const counter = (std::atomic_uint*) buffer;
        if (--(*counter) == 0)
          free(buffer);
      }
    }

    inline Buffer& __attribute__((__always_inline__)) operator=(const Buffer& from) noexcept {
      buffer = from.buffer;

      if (buffer) {
        std::atomic_uint * const counter = (std::atomic_uint*) buffer;
        (*counter)++;
      }
    }

    inline Buffer& __attribute__((__always_inline__)) operator=(Buffer&& from) noexcept {
      void *tmp = buffer;

      buffer = from.buffer;
      from.buffer = tmp;

      return *this;
    }

    inline bool __attribute__((__always_inline__)) isAllocated() noexcept {
      return buffer != NULL;
    }

    inline void* __attribute__((__always_inline__)) ptr() noexcept {
      return isAllocated() ? buffer + sizeof(std::atomic_uint) + sizeof(size_t) : NULL;
    }

    inline char* __attribute__((__always_inline__)) c_str() noexcept {
      return (char*)ptr();
    }

    inline size_t __attribute__((__always_inline__)) size() noexcept {
      return isAllocated() ? *(size_t*)(buffer + sizeof(std::atomic_uint)) : 0;
    }

  private:
    void *buffer;
};

class CriticalSection {
  public:
    CriticalSection() noexcept : lockFlag(ATOMIC_FLAG_INIT) {}

    inline void __attribute__((__always_inline__)) enter() noexcept {
      while(lockFlag.test_and_set())
        ;
    }

    inline bool __attribute__((__always_inline__)) tryLock() noexcept {
      return !lockFlag.test_and_set();
    }

    inline void __attribute__((__always_inline__)) leave() noexcept {
      lockFlag.clear();
    }

  private:
    std::atomic_flag lockFlag;
};

class CriticalSectionHolder {
  public:
    inline __attribute__((__always_inline__)) CriticalSectionHolder(CriticalSection& section) noexcept : section(section) {
      section.enter();
    }

    inline __attribute__((__always_inline__)) ~CriticalSectionHolder() {
      section.leave();
    }

  private:
    CriticalSection& section;
};

class TryCriticalSectionHolder {
  public:
    inline __attribute__((__always_inline__)) TryCriticalSectionHolder(CriticalSection& section) noexcept : section(section) {
    }

    inline __attribute__((__always_inline__)) ~TryCriticalSectionHolder() {
      section.leave();
    }

  private:
    CriticalSection& section;
};

#define CRITICAL(section, block) { CriticalSectionHolder UNIQUE_NAME(cr_sect)(section); block }

#define TRY_CRITICAL(section, block) { if (section.tryLock()) { TryCriticalSectionHolder UNIQUE_NAME(try_cr_sect)(section); block } }

#define POOL_TERMINATOR 0xFFFFFFFF

class MemoryPool;
class MemoryBuffer {
  public:
    inline __attribute__((__always_inline__)) MemoryBuffer(MemoryPool * const p, uint cnt = 1) noexcept : pool(p), counter(cnt) {
    }

  volatile uint32_t nextKey;
  std::atomic_uint counter;
  MemoryPool * const pool;
  uint8_t ptr[];
};

class MemoryBufferHolder {
  private:
    inline void __attribute__((__always_inline__)) tryRelease() noexcept {
      if (buffer && --(buffer->counter) == 0)
        release();
    }

  public:
    inline __attribute__((__always_inline__)) MemoryBufferHolder() noexcept : buffer(NULL) {
    }

    inline __attribute__((__always_inline__)) MemoryBufferHolder(MemoryBuffer *buff) noexcept : buffer(buff) {
    }

    inline __attribute__((__always_inline__)) MemoryBufferHolder(const MemoryBufferHolder& from) noexcept : buffer(from.buffer) {
      if (buffer)
        (buffer->counter)++;      
    }

    inline __attribute__((__always_inline__)) MemoryBufferHolder(MemoryBufferHolder&& from) noexcept {
      MemoryBuffer * const tmp = buffer;

      buffer = from.buffer;
      from.buffer = tmp;
    }

    __attribute__((__always_inline__)) ~MemoryBufferHolder() noexcept {
      tryRelease();
    }

    inline MemoryBufferHolder& __attribute__((__always_inline__)) operator=(const MemoryBufferHolder& from) noexcept {
      tryRelease();

      if ((buffer = from.buffer) != NULL)
        (buffer->counter)++;

      return *this;
    }

    inline MemoryBufferHolder& __attribute__((__always_inline__)) operator=(MemoryBufferHolder&& from) noexcept {
      MemoryBuffer * const tmp = buffer;

      buffer = from.buffer;
      from.buffer = tmp;

      return *this;
    }

    inline bool __attribute__((__always_inline__)) isAllocated() noexcept {
      return buffer != NULL;
    }

    inline void* __attribute__((__always_inline__)) ptr() noexcept {
      return isAllocated() ? (void*)buffer->ptr : NULL;
    }

    inline char* __attribute__((__always_inline__)) c_str() noexcept {
      return (char*)ptr();
    }

    inline size_t __attribute__((__always_inline__)) size() noexcept {
      return isAllocated() ? sizeInt() : 0;
    }

  private:
    size_t sizeInt() noexcept;
    void release() noexcept;

    MemoryBuffer *buffer;  
};

inline size_t __attribute__((__always_inline__)) align_size(size_t size) {
  const size_t align = alignof(void*);
  const size_t dv = size % align;
  return dv ? size + (align - dv) : size;
}

inline void* __attribute__((__always_inline__)) a_alloc(size_t size) {
  return aligned_alloc(alignof(void*), size);
}

class MemoryPool {
  public:
    MemoryPool(size_t eSize, uint16_t cnt) noexcept : first(POOL_TERMINATOR), elementSize(align_size(eSize + sizeof(MemoryBuffer))), count(cnt), firstUnallocatedIndex(0),
                                                    abaProtectCounter(0), buffer((uint8_t*)a_alloc(elementSize * count)) {
    }

    ~MemoryPool() noexcept {
      if (buffer)
        free(buffer);
    }

    inline MemoryBufferHolder __attribute__((__always_inline__)) accure(uint refCounter = 1) noexcept {
      uint32_t firstKey;
      uint8_t *addr;
      size_t index;

      if (buffer != NULL) {
        while ((firstKey = first) != POOL_TERMINATOR) {
          MemoryBuffer *b = (MemoryBuffer*)(buffer + (firstKey & 0x0000FFFF) * elementSize);

          if (first.compare_exchange_strong(firstKey, b->nextKey)) {
            b->counter = refCounter;
            return MemoryBufferHolder(b);
          }
        }

        while((index = firstUnallocatedIndex) < count)
          if (firstUnallocatedIndex.compare_exchange_strong(index, index + 1))
            return MemoryBufferHolder(new (buffer + index * elementSize) MemoryBuffer(this, refCounter));
      }

      return MemoryBufferHolder(NULL);
    }

    inline void __attribute__((__always_inline__)) release(MemoryBuffer *buff) noexcept {
      if (buffer != NULL) {
        const uint32_t newKey = (((uint32_t)(((uint8_t*)buff) - buffer)) / elementSize) | abaProtectCounter.fetch_add(0x00010000);
        uint32_t nextKey;

        do {
          buff->nextKey = nextKey = first;
        } while(!first.compare_exchange_strong(nextKey, newKey));
      }
    }

    inline size_t __attribute__((__always_inline__)) getElementSize() noexcept {
      return elementSize - sizeof(MemoryBuffer);
    }

  private:
    const size_t count;
    const size_t elementSize;
    std::atomic_uint32_t first;
    std::atomic_uint32_t abaProtectCounter;
    std::atomic_size_t firstUnallocatedIndex;
    uint8_t * const buffer;
};

template<typename ITEM_TYPE, size_t MAX_SIZE>
class ArrayList {
  typedef void (*IterateFunc)(ITEM_TYPE& item);

  public:
    inline __attribute__((__always_inline__)) ArrayList() noexcept : num(0) {
    }

    inline bool __attribute__((__always_inline__)) add(ITEM_TYPE item) noexcept {
      critical.enter();

      if (num < MAX_SIZE - 1) {
        items[num++] = item;

        critical.leave();
        return true;
      }

      critical.leave();
      return false;
    }

    inline bool __attribute__((__always_inline__)) remove(ITEM_TYPE item) noexcept {
      critical.enter();
      for (size_t i = 0; i < num; i++)
        if (items[i] == item) {
          if (i < num - 1)
            items[i] = items[num - 1];

          num--;

          critical.leave();
          return true;
        }

      critical.leave();
      return false;
    }

    inline void __attribute__((__always_inline__)) iterate(std::function<void(ITEM_TYPE&)> func) noexcept {
      critical.enter();
      const size_t n = num;

      if (n > 0) {
        ITEM_TYPE copy[MAX_SIZE];

        for (size_t i = 0; i < n; i++)
          copy[i] = items[i];

        critical.leave();

        for (size_t i = 0; i < n; i++)
          func(copy[i]);

        return;
      }

      critical.leave();
    }

  private:
    ITEM_TYPE items[MAX_SIZE];
    CriticalSection critical;
    size_t num;
};

extern std::atomic_uint pwmChannelCounter;
inline uint8_t __attribute__((__always_inline__)) takePwmChannel() noexcept {
  const uint result = pwmChannelCounter++;

  return result >= 16 ? 0xFF : (uint8_t) result;
}

#endif
