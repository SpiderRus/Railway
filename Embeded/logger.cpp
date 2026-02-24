#include "logger.hpp"

#define LOG_HEADER_SIZE 70

#define MAX_LOG_SIZE 512
#define MAX_LOGS 25

static const char level_names[] PROGMEM = {
  "DIWE"
};

void Logger::logTask(void *args) noexcept {
  Logger& logger = *(Logger*)args;
  static struct tm t;

  while(true) {
    MemoryBufferHolder holder;

    if (xQueueReceive(logger.queue, &holder, portMAX_DELAY)) {
      LOG_DATA* const data = (LOG_DATA*)holder.ptr();

      if (logger.level | SP_LOG_UDP) {
        const char * const hostname = logger.config.getHostname();

        logger.udp.writeTo(sizeof(UDP_LOG_MESSAGE) + s_len(logger.config.getHostname()) + s_len(data->tag) + data->length + 2, [hostname, data] (void *ptr, size_t maxLen) {
            UDP_LOG_MESSAGE *msg = (UDP_LOG_MESSAGE*)ptr;
            msg->code = UDP_LOG_CODE;
            msg->level = level_names[data->level];
            msg->stamp = 1000 * ((uint64_t)data->time.tv_sec) + data->time.tv_usec / 1000;

            memcpy(copy_str(copy_str(msg->data, hostname) + 1, data->tag) + 1, data->data, data->length);
        });
      }

      if (logger.level | SP_LOG_CONSOLE) {
        t = *localtime(&data->time.tv_sec);
        Serial.printf("%.2d.%.2d.%.4d %.2d:%.2d:%.2d,%.3d [%c] [%s] ", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec, 
                                            (int)(data->time.tv_usec / 1000), level_names[data->level], data->tag);
                                          
        Serial.write(data->data, data->length);
        Serial.println();
      }       
    }
  }
}

Logger *currentLogger = NULL;

Logger(uint type, Configuration& config, uint numWriters, ...) noexcept :
        type(type),
        config(c),
        level(SP_LOG_LEVEL_INFO),
        memoryPool(MAX_LOG_SIZE + sizeof(LOG_DATA), MAX_LOGS) {
  currentLogger = this;

  if (type | SP_LOG_CONSOLE)
    Serial.begin(115200);

  queue = xQueueCreate(MAX_LOGS, sizeof(MemoryBufferHolder));
  xTaskCreate(logTask, "LOG", 4096, this, tskIDLE_PRIORITY, &taskHandle);
}

void Logger::log(uint level, const char *tag, const char *format, ... ) noexcept {
  if (level) {
    MemoryBufferHolder holder = memoryPool.accure(2);

    if (holder.isAllocated()) {
      LOG_DATA * const data = (LOG_DATA*) holder.ptr();

      gettimeofday(&data->time, NULL);
      data->level = (uint8_t)level;
      data->tag = tag;

      va_list args;
      va_start(args, format);
      data->length = vsnprintf(data->data, MAX_LOG_SIZE, format, args);
      va_end(args);

      xQueueSend(queue, &holder, 0);
    }
  }
}

void Logger::logISR(uint level, const char *tag, const char *format, ... ) noexcept {
  if (level) {
    MemoryBufferHolder holder = memoryPool.accure(2);

    if (holder.isAllocated()) {
      LOG_DATA * const data = (LOG_DATA*) holder.ptr();

      gettimeofday(&data->time, NULL);
      data->level = (uint8_t)level;
      data->tag = tag;

      va_list args;
      va_start(args, format);
      data->length = vsnprintf(data->data, MAX_LOG_SIZE, format, args);
      va_end(args);

      xQueueSendFromISR(queue, &holder, 0);
    }
  }
}

