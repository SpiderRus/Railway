package org.spider.railway.messaging;

import lombok.extern.slf4j.Slf4j;
import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.message.HandshakeMessage;
import org.spider.railway.messaging.message.LogMessage;
import org.spider.railway.messaging.message.StatusMessage;
import org.spider.railway.utils.ReactorUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Lazy;
import org.springframework.stereotype.Service;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Scheduler;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;

import java.util.List;
import java.util.function.Function;

@Slf4j
@Service
@Lazy
public class MessagingServiceAggregator implements MessagingService {
    private final List<MessagingService> subservices;

    @Autowired
    public MessagingServiceAggregator(@NonNull List<MessagingService> subservices) {
        this.subservices = subservices;
    }

    @Override
    @NonNull
    public Flux<? extends HandshakeMessage> subscribeHandshake() {
        return aggregate(MessagingService::subscribeHandshake);
    }

    @Override
    @NonNull
    public Flux<? extends LogMessage> subscribeLog() {
        return aggregate(MessagingService::subscribeLog);
    }

    @Override
    @NonNull
    public Flux<? extends StatusMessage> subscribeStatus() {
        return aggregate(MessagingService::subscribeStatus);
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull SchemeItem.Type type) {
        return aggregate(it -> it.subscribeStatus(type));
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType) {
        return aggregate(it -> it.subscribeStatus(messageType));
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType, @NonNull Scheduler scheduler) {
        return ReactorUtils.merge(subservices.stream().map(it -> it.subscribeStatus(messageType, scheduler)));
    }

    @NonNull
    private <T> Flux<T> aggregate(@NonNull Function<MessagingService, Flux<T>> func) {
        return subservices.size() == 1
                ? func.apply(subservices.get(0))
                : ReactorUtils.publishSingle(ReactorUtils.merge(subservices.stream().map(func)));
    }

    @NonNull
    private Mono<Void> aggregateMono(@NonNull Function<MessagingService, Mono<Void>> func) {
        return subservices.size() == 1
                ? func.apply(subservices.get(0))
                : ReactorUtils.whenAsync(subservices.stream().map(func));
    }

    @Override
    @NonNull
    public <T> Mono<Void> send(@NonNull String clientId, @NonNull T object) {
        return aggregateMono(it -> it.send(clientId, object));
    }

    @Override
    @NonNull
    public Mono<Void> sendPower(@NonNull String clientId, double power) {
        return aggregateMono(it -> it.sendPower(clientId, power));
    }

    @Override
    @NonNull
    public Mono<Void> sendSpeed(@NonNull String clientId, double speed) {
        return aggregateMono(it -> it.sendSpeed(clientId, speed));
    }

    @Override
    @NonNull
    public Mono<Void> sendSwitch(@NonNull String clientId, boolean value) {
        return aggregateMono(it -> it.sendSwitch(clientId, value));
    }

    @Override
    @NonNull
    public Mono<Void> sendSemaphore(@NonNull String clientId, int color) {
        return aggregateMono(it -> it.sendSemaphore(clientId, color));
    }

    @Override
    @Nullable
    public String getClientIdByItemId(@NonNull String itemId) {
        for (MessagingService subservice : subservices) {
            final String clientId = subservice.getClientIdByItemId(itemId);

            if (clientId != null)
                return clientId;
        }

        return null;
    }
}
