package org.spider.railway.messaging.message;

import org.spider.railway.item.SchemeItem;
import reactor.util.annotation.NonNull;

public interface HandshakeMessage extends MessageBase {
    @NonNull
    SchemeItem.Type getType();
}
