package org.spider.railway.controller;

import lombok.extern.slf4j.Slf4j;
import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.MessageStoreService;
import org.spider.railway.messaging.message.StatusMessage;
import org.spider.railway.messaging.message.SwitchStatusMessage;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;

import java.time.OffsetDateTime;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.atomic.AtomicLong;


@RestController
@RequestMapping("/api/scheme-items")
@Slf4j
public class SchemeItemController {
    private final MessageStoreService messageStoreService;

    private static final AtomicLong testSwitchVersion = new AtomicLong();

    @NonNull
    private static SwitchStatusMessage getTestSwitch() {
        final ThreadLocalRandom rnd = ThreadLocalRandom.current();

        return new SwitchStatusMessage() {
            final boolean state = rnd.nextBoolean();
            final OffsetDateTime now = OffsetDateTime.now();
            final double temp = rnd.nextDouble(100.0);

            final long version = testSwitchVersion.incrementAndGet();

            @Override
            public boolean getState() {
                return state;
            }

            @Override
            @NonNull
            public OffsetDateTime getTimestamp() {
                return now;
            }

            @Override
            public double getInternalTemp() {
                return temp;
            }

            @Override
            public long getVersion() {
                return version;
            }

            @Override
            @NonNull
            public OffsetDateTime getCorrectedTimestamp() {
                return now;
            }

            @Override
            @NonNull
            public String getId() {
                return "TestSwitch";
            }

            @Override
            @NonNull
            public OffsetDateTime getReceiveTimestamp() {
                return now;
            }
        };
    }

    @Autowired
    public SchemeItemController(MessageStoreService messageStoreService) {
        this.messageStoreService = messageStoreService;
    }

    @GetMapping(value = "all", produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Flux<StatusMessage> getAll() {
        return Flux.concat(messageStoreService.getAllItems(), Mono.just(getTestSwitch()));
    }
}
