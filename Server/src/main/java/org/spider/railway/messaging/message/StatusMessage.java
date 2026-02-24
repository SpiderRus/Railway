package org.spider.railway.messaging.message;

import org.spider.railway.item.SchemeItem;
import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;
import java.time.ZonedDateTime;

public interface StatusMessage extends MessageBase {
    @NonNull
    OffsetDateTime getTimestamp();

    @NonNull
    SchemeItem.Type getItemType();

    double getInternalTemp();

    long getVersion();

    @NonNull
    OffsetDateTime getCorrectedTimestamp();

    default long getCorrectedTimestampMillis() {
        return getCorrectedTimestamp().toInstant().toEpochMilli();
    }
}
