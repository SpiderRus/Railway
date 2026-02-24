package org.spider.railway.messaging;

import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.message.HandshakeMessage;
import org.spider.railway.messaging.message.LogMessage;
import org.spider.railway.messaging.message.StatusMessage;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Scheduler;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;

public interface MessagingService {
    @NonNull
    Flux<? extends HandshakeMessage> subscribeHandshake();

    @NonNull
    Flux<? extends LogMessage> subscribeLog();

    @NonNull
    Flux<? extends StatusMessage> subscribeStatus();

    @NonNull
    <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull SchemeItem.Type type);

    @NonNull
    <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType);

    @NonNull
    <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType, @NonNull Scheduler scheduler);

    @NonNull
    <T> Mono<Void> send(@NonNull String itemId, @NonNull T object);

    @NonNull
    Mono<Void> sendPower(@NonNull String itemId, double power);

    @NonNull
    Mono<Void> sendSpeed(@NonNull String itemId, double speed);

    @NonNull
    Mono<Void> sendSwitch(@NonNull String itemId, boolean value);

    @NonNull
    Mono<Void> sendSemaphore(@NonNull String itemId, int color);

    @Nullable
    String getClientIdByItemId(@NonNull String itemId);
}
