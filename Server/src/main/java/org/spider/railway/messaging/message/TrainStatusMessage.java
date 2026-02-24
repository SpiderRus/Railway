package org.spider.railway.messaging.message;

import org.spider.railway.item.SchemeItem;
import reactor.util.annotation.NonNull;

public interface TrainStatusMessage extends MarkerMessage {
    @Override
    @NonNull
    default SchemeItem.Type getItemType() {
        return SchemeItem.Type.TRAIN;
    }

    int getSpeed();

    double getPower();
}
