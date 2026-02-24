package org.spider.railway.train;

import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.Getter;
import lombok.ToString;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.exception.NotFoundException;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.messaging.message.TrainStatusMessage;
import org.spider.railway.scheme.SchemeCoords;
import org.spider.railway.scheme.SchemeMarker;
import org.spider.railway.scheme.SchemeService;
import org.spider.railway.utils.Utils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import reactor.core.Disposable;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;

@Slf4j
@Service
public class TrainService {
    @Getter
    private class Train {
        @Getter
        @ToString
        private static class Status {
            private final TrainStatusMessage lastMessage;
            private final SchemeCoords coords;

            Status(@NonNull TrainStatusMessage message, @NonNull SchemeCoords coords) {
                this.lastMessage = message;
                this.coords = coords;
            }
        }

        private final AtomicReference<Status> status;

        Train(@NonNull TrainStatusMessage message) {
            this.status = new AtomicReference<>(new Status(message, schemeService.getStartPosition()));
        }

        public void updateStatus(@NonNull TrainStatusMessage message) {
            Status oldStatus, newStatus;

            do {
                oldStatus = this.status.get();

                final TrainStatusMessage lastMessage = oldStatus.getLastMessage();
                final double avgSpeed = (message.getSpeed() + lastMessage.getSpeed()) / 2.0;
                final int distance = (int) (avgSpeed *
                        (message.getCorrectedTimestampMillis() - lastMessage.getCorrectedTimestampMillis()) / 1000.0);

                final SchemeCoords newCoords = null;
                final SchemeCoords resultCoords;
                if (newCoords != null) {
                    if (message.getSchemeMarkerTime() == null
                            || (lastMessage.getSchemeMarkerTime() != null
                                && Utils.isAfterOrEquals(lastMessage.getSchemeMarkerTime(), message.getSchemeMarkerTime()))) {
                        // no new marker reached
                        resultCoords = newCoords;
                    } else {
                        // new marker reached
                        final SchemeMarker marker = null;

                        if (marker != null)
                            resultCoords = marker.getCoords();
                        else {
                            log.warn("Can't find marker: coords={}", newCoords);
                            resultCoords = newCoords;
                        }
                    }
                } else {
                    log.warn("Can't find new position: coords={}, distance={}", oldStatus.getCoords(), distance);
                    resultCoords = oldStatus.getCoords();
                }

                newStatus = new Status(message, resultCoords);
            } while (!this.status.compareAndSet(oldStatus, newStatus));
        }
    }

    private final MessagingService messagingService;

    private final SchemeService schemeService;

    private Disposable disposable;

    private final ConcurrentHashMap<String, Train> trainsById = new ConcurrentHashMap<>();

    @Autowired
    public TrainService(MessagingService messagingService, SchemeService schemeService) {
        this.messagingService = messagingService;
        this.schemeService = schemeService;
    }

    @PostConstruct
    private void init() {
        this.disposable = this.messagingService
                .subscribeStatus(TrainStatusMessage.class)
                .concatMap(msg -> {
                    Train train = trainsById.get(msg.getId());

                    if (train == null)
                        train = trainsById.putIfAbsent(msg.getId(), new Train(msg));

                    if (train != null) {
                        train.updateStatus(msg);
                        // log.info("Update train status: {} {}", msg.getId(), train.getStatus());
                    }

                    return Mono.empty();
                })
                .subscribe();
    }

    @PreDestroy
    private void destroy() {
        this.disposable.dispose();
    }

    @NonNull
    public Mono<Void> setSpeed(@NonNull String id, @NonNull double speed) {
        if (!trainsById.containsKey(id))
            return Mono.error(new NotFoundException("Train with id " + id + " not found"));

        return messagingService.sendSpeed(id, speed);
    }
}
