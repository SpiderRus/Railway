#ifndef __QUEUE_WORKER_H
#define __QUEUE_WORKER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

class QueueWorkerBase {
  private:
    static void taskStatic(void *arg) noexcept;

  protected:
    QueueWorkerBase(size_t queueSize, const char *name, unsigned int priority, size_t elementSize, size_t stackSize = 4096) noexcept :
            name(name),
            priority(priority),
            queue(xQueueCreate(queueSize, elementSize)) {
        xTaskCreate(taskStatic, name, stackSize, this, tskIDLE_PRIORITY, &taskHandle);
    }

    virtual void task() noexcept = 0;

    inline bool __attribute__((__always_inline__)) receiveQueueItem(void *data) noexcept {
        return xQueueReceive(queue, data, portMAX_DELAY);
    }

    inline bool __attribute__((__always_inline__)) receiveQueueItem(void *data, int32_t delayMs) noexcept {
        return xQueueReceive(queue, data, likely(delayMs <= 0) ? 0 : pdMS_TO_TICKS(delayMs));
    }

    inline bool __attribute__((__always_inline__)) sendQueueItem(void *data) noexcept {
        return xQueueSend(queue, data, 0) == pdPASS;
    }

    inline bool __attribute__((__always_inline__)) sendQueueItemFromISR(void *data, BaseType_t * const priority) noexcept {
        return xQueueSendFromISR(queue, data, priority) == pdPASS;
    }

    inline bool __attribute__((__always_inline__)) overwriteQueueItem(void *data) noexcept {
        return xQueueOverwrite(queue, data) == pdPASS;
    }

    inline bool __attribute__((__always_inline__)) overwriteQueueItemFromISR(void *data, BaseType_t * const priority) noexcept {
        return xQueueOverwriteFromISR(queue, data, priority) == pdPASS;
    }

  protected:
    TaskHandle_t taskHandle;
    QueueHandle_t queue;

  private:
    const unsigned int priority;
    const char * const name;
};

template<typename T>
class QueueWorker : protected QueueWorkerBase {
protected:
    QueueWorker(size_t queueSize, const char *name, unsigned int priority, size_t stackSize = 4096) noexcept : 
        QueueWorkerBase(queueSize, name, priority, sizeof(T), stackSize) {}

    void IRAM_ATTR task() noexcept override final {
        do {
            T data;
            
            if (receiveQueueItem(data))
                processQueueItem(data);
        } while (true);
    }

    inline bool __attribute__((__always_inline__)) receiveQueueItem(T& data) noexcept {
        return QueueWorkerBase::receiveQueueItem(&data);
    }

    inline bool __attribute__((__always_inline__)) receiveQueueItem(T& data, int32_t delayMs) noexcept {
        return QueueWorkerBase::receiveQueueItem(&data, delayMs);
    }

    inline bool __attribute__((__always_inline__)) sendQueueItem(T& data) noexcept {
        return QueueWorkerBase::sendQueueItem(&data);
    }

    inline bool __attribute__((__always_inline__)) sendQueueItemFromISR(T& data, BaseType_t * const priority) noexcept {
        return QueueWorkerBase::sendQueueItemFromISR(&data, priority);
    }

    inline bool __attribute__((__always_inline__)) overwriteQueueItem(T& data) noexcept {
        return QueueWorkerBase::overwriteQueueItem(&data);
    }

    inline bool __attribute__((__always_inline__)) overwriteQueueItemFromISR(T& data, BaseType_t * const priority) noexcept {
        return QueueWorkerBase::overwriteQueueItemFromISR(&data, priority);
    }

    virtual void processQueueItem(T& data) noexcept = 0;
};

#endif // __QUEUE_WORKER_H
