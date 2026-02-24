package org.spider.railway.messaging.message;

import org.spider.railway.item.SchemeItem;
import reactor.util.annotation.NonNull;

public interface SemaphoreStatusMessage extends StatusMessage {
    int getColor();

    @Override
    @NonNull
    default SchemeItem.Type getItemType() {
        return SchemeItem.Type.SEMAPHORE;
    }
}
