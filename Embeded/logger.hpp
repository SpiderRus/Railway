#ifndef INC_LOGGER_HPP
#define INC_LOGGER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Utils.hpp"
#include "configuration.hpp"

#define SP_LOG_CONSOLE 1
#define SP_LOG_MQTT    2
#define SP_LOG_UDP     4

#define SP_LOG_LEVEL_DEBUG    0
#define SP_LOG_LEVEL_INFO     1
#define SP_LOG_LEVEL_WARNING  2
#define SP_LOG_LEVEL_ERROR    3

#define SP_LOG(level, tag, ... ) { if (currentLogger && (currentLogger->getLevel() <= level)) { currentLogger->log(level, tag, ##__VA_ARGS__); } }

#define SP_LOG_ISR(level, tag, ... ) { if (currentLogger && (currentLogger->getLevel() <= level)) { currentLogger->logISR(level, tag, ##__VA_ARGS__); } }

#define SP_LOGD(tag, ... ) SP_LOG(SP_LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define SP_LOGI(tag, ... ) SP_LOG(SP_LOG_LEVEL_INFO, tag,  __VA_ARGS__)
#define SP_LOGW(tag, ... ) SP_LOG(SP_LOG_LEVEL_WARNING, tag,  __VA_ARGS__)
#define SP_LOGE(tag, ... ) SP_LOG(SP_LOG_LEVEL_ERROR, tag, __VA_ARGS__)

#define SP_LOGD_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define SP_LOGI_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_INFO, tag,  __VA_ARGS__)
#define SP_LOGW_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_WARNING, tag,  __VA_ARGS__)
#define SP_LOGE_ISR(tag, ... ) SP_LOG_ISR(SP_LOG_LEVEL_ERROR, tag, __VA_ARGS__)

typedef struct _LOG_DATA {
  struct timeval time;
  uint8_t level;
  const char *tag;
  size_t length;
  char data[];
} LOG_DATA;

class LogWriter {
  public:
    virtual void write(MemoryBufferHolder& logDataHolder) noexcept;
}

class Logger {
  public:
    Logger(uint type, Configuration& config, uint numWriters, ...) noexcept;

    void log(uint level, const char *tag, const char *format, ... ) noexcept;

    void logISR(uint level, const char *tag, const char *format, ... ) noexcept;

    inline uint __attribute__((__always_inline__)) getLevel() noexcept { return level; }
  private:
    const uint type;
    const uint level;
    Configuration& config;
    TaskHandle_t taskHandle;
    QueueHandle_t queue;
    MemoryPool memoryPool;
    ArrayList<LogWriter&, 16> writers;

    static void logTask(void *args) noexcept;
};

extern Logger *currentLogger;

#endif

