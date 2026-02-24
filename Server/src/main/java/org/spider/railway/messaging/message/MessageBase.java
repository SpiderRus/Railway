package org.spider.railway.messaging.message;

import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;

public interface MessageBase {
    @NonNull
    String getId();

    @NonNull
    OffsetDateTime getReceiveTimestamp();
}
