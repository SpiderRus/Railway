package org.spider.railway.messaging.message;

import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;

public abstract class AbstractMessage implements MessageBase {
    private final String id;

    private final OffsetDateTime receiveTimestamp = OffsetDateTime.now();

    protected AbstractMessage(@NonNull String id) {
        this.id = id;
    }

    @Override
    @NonNull
    public String getId() {
        return this.id;
    }

    @Override
    @NonNull
    public OffsetDateTime getReceiveTimestamp() {
        return this.receiveTimestamp;
    }
}
