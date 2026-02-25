#ifndef __LOGGER_H
#define __LOGGER_H

#include <sys/_stdint.h>
#include <memory.h>
#include <utils/PtrHolder.h>
#include <utils/ObjectPool.h>
#include <Arduino.h>
#include <udp/udp.h>
#include <utils/Utils.h>
#include <utils/QueueWorker.h>

#define SP_LOG_LEVEL_DEBUG    0
#define SP_LOG_LEVEL_INFO     1
#define SP_LOG_LEVEL_WARNING  2
#define SP_LOG_LEVEL_ERROR    3

#define SP_LOG(level, tag, ... ) { if (likely(currentLogger && (currentLogger->getLevel() <= level))) { currentLogger->log(level, tag, ##__VA_ARGS__); } }

#define SP_LOG_ISR(level, tag, ... ) { if (likely(currentLogger && (currentLogger->getLevel() <= level))) { currentLogger->logISR(level, tag, ##__VA_ARGS__); } }

#define SP_LOGD(tag, ... ) SP_LOG(SP_LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define SP_LOGI(tag, ... ) SP_LOG(SP_LOG_LEVEL_INFO, tag,  __VA_ARGS__)
#define SP_LOGW(tag, ... ) SP_LOG(SP_LOG_LEVEL_WARNING, tag,  __VA_ARGS__)
#define SP_LOGE(tag, ... ) SP_LOG(SP_LOG_LEVEL_ERROR, tag, __VA_ARGS__)

#define SP_LOGD_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define SP_LOGI_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_INFO, tag,  __VA_ARGS__)
#define SP_LOGW_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_WARNING, tag,  __VA_ARGS__)
#define SP_LOGE_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_ERROR, tag, __VA_ARGS__)

#define LOG_DATA_SIZE 256

#define MAX_LOGS 40

class LogData : public PooledObject<LogData> {
public:
    inline __attribute__((__always_inline__)) explicit LogData(ObjectPool<LogData>& p) noexcept : PooledObject<LogData>(p) {}

public:
    uint64_t timeMs;
    uint8_t core;
    int8_t priority;
    uint8_t level;
    const char *tag;
    size_t messageLength;
    char message[LOG_DATA_SIZE];
};

class LoggerObjectPool {
public:
    LoggerObjectPool(uint16_t size) noexcept : pool(size) {}

    inline PtrHolder<LogData> __attribute__((__always_inline__)) poll() noexcept {
        LogData *ptr = pool.poll();

        return unlikely(ptr == nullptr) ? PtrHolder<LogData>() : PtrHolder(new (ptr) LogData(pool));
    }

private:
    ObjectPool<LogData> pool;
};

class LogWriter {
public:
    virtual void log(PtrHolder<LogData>& logData) noexcept;
};

class ConsoleLogger : public LogWriter {
public:
    void log(PtrHolder<LogData>& logData) noexcept;
};

class UdpLogger : public LogWriter {
public:
    UdpLogger(UDPSender &sender) noexcept : sender(sender) {}

    void log(PtrHolder<LogData>& logData) noexcept;
private:
    static int16_t IRAM_ATTR callbackStatic(void *args, uint8_t *buffer, size_t maxLen);

    UDPSender &sender;
};

class Logger : protected QueueWorkerBase {
public:
    Logger(uint numWriters, ...) noexcept :
                    QueueWorkerBase(MAX_LOGS, "LOG", LOG_TASK_PRIORITY, sizeof(PtrHolder<LogData>)),
                    level(SP_LOG_LEVEL_DEBUG),
                    pool(MAX_LOGS) {
        writers = (LogWriter**) malloc((numWriters + 1) * sizeof(LogWriter*));

        va_list args;
        va_start(args, numWriters);
        for (uint i = 0; i < numWriters; i++)
            writers[i] = va_arg(args, LogWriter*);
        va_end(args);

        writers[numWriters] = nullptr;
    }

    ~Logger() noexcept {
        free(writers);
    }

    inline uint __attribute__((__always_inline__)) getLevel() const noexcept {
        return level;
    }

    void IRAM_ATTR log(uint level, const char *tag, const char *format, ... ) noexcept;

    //void logISR(uint level, const char *tag, const char *format, ... ) noexcept;

protected:
    void IRAM_ATTR task() noexcept override;

private:
    LogWriter **writers;
    LoggerObjectPool pool;
    const uint level;
};

extern Logger *currentLogger;

extern const char level_names[];
#endif
