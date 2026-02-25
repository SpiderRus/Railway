#include <utils/QueueWorker.h>

void QueueWorkerBase::taskStatic(void *arg) noexcept {
    QueueWorkerBase* const ptr = (QueueWorkerBase*) arg;

    vTaskPrioritySet(NULL, ptr->priority);

    ptr->task();
}
