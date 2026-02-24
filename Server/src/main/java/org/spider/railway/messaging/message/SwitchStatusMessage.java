package org.spider.railway.messaging.message;

import org.spider.railway.item.SchemeItem;
import reactor.util.annotation.NonNull;

public interface SwitchStatusMessage extends StatusMessage {
    boolean getState();

    @Override
    @NonNull
    default SchemeItem.Type getItemType() {
        return SchemeItem.Type.SWITCH;
    }
}
