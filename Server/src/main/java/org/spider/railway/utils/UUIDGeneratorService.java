package org.spider.railway.utils;

import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.r2dbc.core.ConnectionAccessor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.ReactiveTransactionManager;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Scheduler;
import reactor.core.scheduler.Schedulers;

import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.Collectors;
import java.util.stream.Stream;

@Slf4j
@Service
public class UUIDGeneratorService {
    private static final int GENERATE_COUNT = 1_000;

    private final ReactiveTransactionManager tm;

    private final ConnectionAccessor connectionAccessor;

    private final SingleLinkedQueue<String> queue = new SingleLinkedQueue<>();

    private Scheduler scheduler;

    private final AtomicBoolean generatorStarted = new AtomicBoolean();

    @Autowired
    public UUIDGeneratorService(ReactiveTransactionManager tm, ConnectionAccessor connectionAccessor) {
        this.tm = tm;
        this.connectionAccessor = connectionAccessor;
    }

    @PostConstruct
    private void init() {
        scheduler = Schedulers.single(Schedulers.boundedElastic());
        tryGenerate();
    }

    @PreDestroy
    private void destroy() {
        scheduler.dispose();
    }

    public String generate() {
        String result = queue.poll();

        if (result == null || queue.getSize() < GENERATE_COUNT >> 1) {
            tryGenerate();
            return result == null ? UUID.randomUUID().toString() : result;
        }

        return result;
    }

    @Transactional(propagation = Propagation.REQUIRES_NEW)
    public Mono<Void> checkTransaction() {
        return Mono.when(
                Stream.of("10", "20", "30", "40", "50")
                        .map(k -> DBUtils.testTransact(k, connectionAccessor))
                        //.map(m -> m.subscribeOn(Schedulers.boundedElastic()))
                        .collect(Collectors.toList())
            );
    }

    private void tryGenerate() {
        if (generatorStarted.compareAndSet(false, true)) {
            scheduler.schedule(() -> {
                int num;

                while ((num = GENERATE_COUNT - queue.getSize()) > 0) {
                    for (int i = 0; i < num; i++)
                        queue.add(UUID.randomUUID().toString());
                }

                generatorStarted.set(false);
            });
        }
    }
}
