package org.spider.railway.messaging.message;

import reactor.util.annotation.NonNull;

import java.net.InetSocketAddress;
import java.time.OffsetDateTime;
import java.time.ZonedDateTime;

public interface LogMessage extends MessageBase {
    @NonNull
    OffsetDateTime getTimestamp();

    int getCore();

    int getLevel();

    @NonNull
    String getTag();

    @NonNull
    String getMessage();

    @NonNull
    InetSocketAddress getAddress();
}
