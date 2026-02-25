#include "HardwareSerial.h"
#include <string.h>
#include <logger/logger.h>
#include <Arduino.h>
#include <utils/Utils.h>

const char level_names[] = {
  "DIWE"
};

Logger *currentLogger = nullptr;

void IRAM_ATTR Logger::task() noexcept {
    do {
        PtrHolder<LogData> holder;
        
        if (receiveQueueItem(&holder)) {
            for (LogWriter **logWriters = writers; *logWriters; logWriters++)
                (*logWriters)->log(holder);
        }
    } while (true);
}

void IRAM_ATTR Logger::log(uint level, const char *tag, const char *format, ... ) noexcept {
    auto logDataHolder = pool.poll();

    if (likely(logDataHolder.isAllocated())) {
        logDataHolder.accure(1);
        LogData * const logData = logDataHolder.ptr();

        logData->timeMs = currentTimeMillis();
        logData->core = (uint8_t) xPortGetCoreID();
        logData->priority = (int8_t) (((int)uxTaskPriorityGet(NULL)) - NORMAL_TASK_PRIORITY);
        logData->level = (uint8_t) level;
        logData->tag = tag;

        va_list args;
        va_start(args, format);
        logData->messageLength = vsnprintf(logData->message, LOG_DATA_SIZE, format, args);
        va_end(args);

        sendQueueItem(&logDataHolder);
    }
}

void ConsoleLogger::log(PtrHolder<LogData>& holder) noexcept {
    static struct tm t;

    LogData *data = holder.ptr();

    const time_t time = (time_t) divBy1000(data->timeMs);
    t = *localtime(&time);
    Serial.printf("%.2d.%.2d.%.4d %.2d:%.2d:%.2d,%.3d [%.1d] [%+.2d] [%c] [%s] ", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec, 
                                    (int) (data->timeMs % 1000), (int)(data->core), (int)(data->priority), level_names[data->level], data->tag);
                                    
    Serial.write(data->message, data->messageLength);
    Serial.println();
}

void UdpLogger::log(PtrHolder<LogData>& holder) noexcept {
    holder.accure(1);

    if (!sender.send(callbackStatic, holder.ptr()))
        holder.release();
}

int16_t IRAM_ATTR UdpLogger::callbackStatic(void *args, uint8_t *buffer, size_t maxLen) {
    PtrHolder<LogData> holder((LogData*)args, 0);
    LogData * const logData = holder.ptr();

    return (int16_t) BinarySerializer::serialize(buffer, maxLen,
        (uint8_t) PACKET_TYPE_LOG,
        (uint64_t) logData->timeMs,
        (uint8_t) logData->core,
        (int8_t) logData->priority,
        (uint8_t) logData->level,
        logData->tag,
        MemBuffer(logData->message, logData->messageLength)
    );
}
