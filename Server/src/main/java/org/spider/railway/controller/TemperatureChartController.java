package org.spider.railway.controller;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.messaging.MessageStoreService;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.statuses.StatusService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;
import java.time.temporal.ChronoUnit;

@RestController
@RequestMapping("/api/temperature-chart")
@Slf4j
public class TemperatureChartController {
    private static final long MAX_MS_IN_CHARTS = 1L * 60_000L;

    @AllArgsConstructor
    @Getter
    public static class TemperaturePoint {
        private final long x;
        @JsonInclude(JsonInclude.Include.ALWAYS)
        private Double y;
    }

    private final StatusService statusService;

    private final MessageStoreService messageStoreService;

    private final MessagingService messagingService;

    @Autowired
    public TemperatureChartController(StatusService statusService,
                                      MessageStoreService messageStoreService,
                                      MessagingService messagingService) {
        this.statusService = statusService;
        this.messageStoreService = messageStoreService;
        this.messagingService = messagingService;
    }

    @GetMapping(value = "all", produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Flux<CharSequence> getAllItems() {
        return messageStoreService.getAllItemIds()
                .flatMap(itemId -> Mono.justOrEmpty(messagingService.getClientIdByItemId(itemId)))
                .distinct()
                .map(it -> it);
    }

    @GetMapping(value = "{itemId}", produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Flux<TemperaturePoint> getChartData(@PathVariable @NonNull String itemId) {
        return Flux.defer(() -> {
            final OffsetDateTime now = OffsetDateTime.now();

            return Flux.concat(
                    Flux.just(new TemperaturePoint(now.toInstant().toEpochMilli() - MAX_MS_IN_CHARTS, null)),
                    statusService.getMessages(itemId, now.minus(MAX_MS_IN_CHARTS, ChronoUnit.MILLIS), now).map(message -> {
                        return new TemperaturePoint(message.getCorrectedTimestampMillis(), message.getInternalTemp());
                    }),
                    Flux.just(new TemperaturePoint(now.toInstant().toEpochMilli(), null)));
        });
    }
}