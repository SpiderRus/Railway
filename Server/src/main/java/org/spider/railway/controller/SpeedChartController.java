package org.spider.railway.controller;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.MessageStoreService;
import org.spider.railway.statuses.StatusService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Flux;
import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;
import java.time.temporal.ChronoUnit;

@RestController
@RequestMapping("/api/speed-chart")
@Slf4j
public class SpeedChartController {
    private static final long MAX_MS_IN_CHARTS = 1L * 60_000L;

    @AllArgsConstructor
    @Getter
    public static class SpeedPoint {
        private final long x;
        @JsonInclude(JsonInclude.Include.ALWAYS)
        private Integer y;
    }

    private final StatusService statusService;

    private final MessageStoreService messageStoreService;

    @Autowired
    public SpeedChartController(StatusService statusService, MessageStoreService messageStoreService) {
        this.statusService = statusService;
        this.messageStoreService = messageStoreService;
    }

    @GetMapping(value = "all", produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Flux<CharSequence> getAllItems() {
        return messageStoreService.getItemIdsForTypes(SchemeItem.Type.TRAIN).map(it -> it);
    }

    @GetMapping(value = "{trainId}", produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Flux<SpeedPoint> getChartData(@PathVariable @NonNull String trainId) {
        return Flux.defer(() -> {
            final OffsetDateTime now = OffsetDateTime.now();

            return Flux.concat(
                    Flux.just(new SpeedPoint(now.toInstant().toEpochMilli() - MAX_MS_IN_CHARTS, null)),
                    statusService.getMessages(trainId, now.minus(MAX_MS_IN_CHARTS, ChronoUnit.MILLIS), now)
                        .filter(message -> message.getItemType() == SchemeItem.Type.TRAIN)
                        .map(message -> {
                            final int speed = (int) ((message.getCorrectedTimestampMillis() / 500) % 100);
                            return new SpeedPoint(message.getCorrectedTimestampMillis(), speed);
                        }),
                    Flux.just(new SpeedPoint(now.toInstant().toEpochMilli(), null)));
        });
    }
}
