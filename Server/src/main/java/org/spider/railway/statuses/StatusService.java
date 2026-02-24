package org.spider.railway.statuses;

import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.messaging.MessageStoreService;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.messaging.message.StatusMessage;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import reactor.core.Disposable;
import reactor.core.publisher.Flux;
import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentHashMap;

@Slf4j
@Service
public class StatusService {
    private static final long MAX_IN_MEMORY_MS = 10 * 60_000L; // 15 min

    private static final int MAX_IN_MEMORY_COUNT = 1_000;

    private final MessagingService messagingService;

    private final MessageStoreService messageStoreService;

    private final ConcurrentHashMap<String, LinkedList<StatusMessage>> statuses = new ConcurrentHashMap<>();

    private Disposable disposable;

    @Autowired
    public StatusService(MessagingService messagingService, MessageStoreService messageStoreService) {
        this.messagingService = messagingService;
        this.messageStoreService = messageStoreService;
    }

    @PostConstruct
    private void init() {
        this.disposable = messagingService
                .subscribeStatus(StatusMessage.class)
                .subscribe(message -> {
                    final LinkedList<StatusMessage> messages = statuses.computeIfAbsent(message.getId(), __ -> new LinkedList<>());

                    synchronized (messages) {
                        if (!messages.isEmpty()
                                && (messages.size() > MAX_IN_MEMORY_COUNT
                                    || (System.currentTimeMillis() - messages.getFirst().getCorrectedTimestampMillis()) > MAX_IN_MEMORY_MS)) {
                            messages.removeFirst();
                        }

                        messages.add(message);
                    }
                });
    }

    @PreDestroy
    private void destroy() {
        this.disposable.dispose();
    }

    @NonNull
    private Flux<StatusMessage> getStoredMessages(@NonNull String trainId, @NonNull OffsetDateTime from, @NonNull OffsetDateTime to) {
        return messageStoreService.getMessages(trainId, from, to).cast(StatusMessage.class);
    }

    @NonNull
    public Flux<StatusMessage> getMessages(@NonNull String trainId, @NonNull OffsetDateTime from, @NonNull OffsetDateTime to) {
        return Flux.defer(() -> {
            final LinkedList<StatusMessage> messages = statuses.get(trainId);

            if (messages == null)
                return getStoredMessages(trainId, from, to);

            final ArrayList<StatusMessage> result;
            final OffsetDateTime firstMessageTime;
            final OffsetDateTime lastMessageTime;

            synchronized (messages) {
                result = new ArrayList<>(messages.size());

                for (StatusMessage message : messages) {
                    final OffsetDateTime time = message.getCorrectedTimestamp();

                    if (time.isAfter(from) || time.isEqual(from))
                        result.add(message);
                    else if (time.isAfter(to))
                        break;
                }

                firstMessageTime = messages.getFirst().getCorrectedTimestamp();
                lastMessageTime = messages.getLast().getCorrectedTimestamp();
            }

            Flux<StatusMessage> resultFlux;
            final Flux<StatusMessage> fromArrayFlux = Flux.fromIterable(result);

            if (from.isBefore(firstMessageTime))
                resultFlux = Flux.concat(getStoredMessages(trainId, from, firstMessageTime.minus(1L, ChronoUnit.MILLIS)), fromArrayFlux);
            else
                resultFlux = fromArrayFlux;

            if (to.isAfter(lastMessageTime) && to.isAfter(OffsetDateTime.now().plus(2L, ChronoUnit.MILLIS)))
                resultFlux = Flux.concat(resultFlux, getStoredMessages(trainId, lastMessageTime.plus(1L, ChronoUnit.MILLIS), to));

            return resultFlux;
        });
    }
}
