package org.spider.railway.messaging.message;

import reactor.util.annotation.Nullable;

import java.time.OffsetDateTime;

public interface MarkerMessage extends StatusMessage {
    @Nullable
    OffsetDateTime getSchemeMarkerTime();
}
