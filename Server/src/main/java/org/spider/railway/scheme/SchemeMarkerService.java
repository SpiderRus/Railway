package org.spider.railway.scheme;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.messaging.message.MarkerMessage;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;

import java.time.OffsetDateTime;
import java.util.concurrent.ConcurrentHashMap;

@Slf4j
@Service
public class SchemeMarkerService {
    private static final long MAX_TIME_DIFF_MS = 10_000;

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class MarkerEvent {
        private final String itemId;
        private final OffsetDateTime time;
    }

    private final MessagingService messagingService;

    private final ConcurrentHashMap<String, OffsetDateTime> onStatus = new ConcurrentHashMap<>();

    @Autowired
    public SchemeMarkerService(MessagingService messagingService) {
        this.messagingService = messagingService;
    }

    public Flux<MarkerEvent> subscribe() {
        return messagingService.subscribeStatus(MarkerMessage.class)
                .concatMap(status -> Mono.justOrEmpty(getEvent(status)));
    }

    @Nullable
    private MarkerEvent getEvent(@NonNull MarkerMessage message) {
        final OffsetDateTime markerTime = message.getSchemeMarkerTime();

        if (markerTime == null)
            onStatus.remove(message.getId());
        else {
            final OffsetDateTime prevTime = onStatus.get(message.getId());

            if (prevTime == null || prevTime.isBefore(markerTime)) {
                onStatus.put(message.getId(), markerTime);
                return new MarkerEvent(message.getId(), markerTime);
            }
        }

        return null;
    }
}
