package org.spider.railway.controller;

import com.fasterxml.jackson.core.JsonProcessingException;
import io.swagger.v3.oas.annotations.media.ArraySchema;
import io.swagger.v3.oas.annotations.media.Content;
import io.swagger.v3.oas.annotations.media.Schema;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.reactivestreams.Publisher;
import org.spider.railway.exception.NotFoundException;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.messaging.message.HandshakeMessage;
import org.spider.railway.train.TrainService;
import org.spider.railway.utils.DBUtils;
import org.spider.railway.utils.UUIDGeneratorService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.r2dbc.core.ConnectionAccessor;
import org.springframework.transaction.ReactiveTransactionManager;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.server.ResponseStatusException;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxSink;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuples;

import java.io.*;
import java.time.OffsetDateTime;
import java.util.Map;
import java.util.concurrent.Flow;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicIntegerFieldUpdater;
import java.util.function.Consumer;
import java.util.stream.Collectors;
import java.util.stream.Stream;

@RestController
@RequestMapping("/api/test")
@Slf4j
public class TestController {

    private final MessagingService messagingService;

    private final UUIDGeneratorService uuidGeneratorService;

    private final TrainService trainService;

    private final ReactiveTransactionManager tm;

    private final ConnectionAccessor connectionAccessor;

    @Autowired
    public TestController(MessagingService messagingService,
                          UUIDGeneratorService uuidGeneratorService,
                          TrainService trainService,
                          ReactiveTransactionManager tm,
                          ConnectionAccessor connectionAccessor) {
        this.messagingService = messagingService;
        this.uuidGeneratorService = uuidGeneratorService;
        this.trainService = trainService;
        this.tm = tm;
        this.connectionAccessor = connectionAccessor;
    }

    private static class LongGenEmitter implements Consumer<FluxSink<Long>> {
        private final long start, end, step;
        private long current;
        private FluxSink<Long> sink;

        LongGenEmitter(long start, long end, long step) {
            this.start = start;
            this.end = end;
            this.step = step;
        }

        @Override
        public void accept(FluxSink<Long> sink) {
            this.sink = sink;
            this.current = this.start;

            sink.onRequest(this::request);
        }

        private void request(long __) {
            for (; sink.requestedFromDownstream() > 0 && current <= end && !sink.isCancelled(); current += step)
                sink.next(current);

            if (current > end && !sink.isCancelled())
                sink.complete();
        }
    }

    private static class LineReader implements Consumer<FluxSink<String>> {
        private final BufferedReader reader;
        private FluxSink<String> sink;

        LineReader(@NonNull BufferedReader reader) {
            this.reader = reader;
        }

        @Override
        public void accept(FluxSink<String> sink) {
            this.sink = sink;

            sink.onRequest(this::request);
        }

        private void request(long __) {
            String line;

            try {
                while ((line = reader.readLine()) != null && sink.requestedFromDownstream() > 0 && !sink.isCancelled())
                    sink.next(line);

                if (line == null && !sink.isCancelled())
                    sink.complete();
            } catch (IOException ioe) {
                sink.error(ioe);
            }
        }

        @NonNull
        public static Flux<String> create(@NonNull BufferedReader reader) {
            return Flux.create(new LineReader(reader));
        }
    }

    @NonNull
    private Flux<String> readFile(@NonNull File file) {
        return Flux.using(() -> new BufferedReader(new FileReader(file)),
                LineReader::create, reader -> {
                    try {
                        //noinspection BlockingMethodInNonBlockingContext
                        reader.close();
                    } catch (IOException ignored) {}
                });
    }

    private static Flux<Long> generate(long from, long to, long step) {
        return Flux.create(new LongGenEmitter(from, to, step));
    }

    public record TimeSeriesResult(long time, double value) { }

    private static final Mono<?> NOT_FOUND = Mono.error(new ResponseStatusException(HttpStatus.NOT_FOUND, "Empty!!!"));

    public static <T> Publisher<T> notFound() {
        //noinspection unchecked
        return (Publisher<T>) NOT_FOUND;
    }

    @GetMapping
    @ApiResponses({
        @ApiResponse(
                responseCode = "200",
                description = "Успешный ответ",
                content = @Content(
                        mediaType = MediaType.APPLICATION_JSON_VALUE,
                        array = @ArraySchema(schema = @Schema(implementation = TimeSeriesResult.class))
                )
        ),
        @ApiResponse(
                responseCode = "404",
                description = "Не найдено",
                content = @Content(
                        mediaType = MediaType.TEXT_PLAIN_VALUE,
                        schema = @Schema(implementation = String.class)
                )
        )
    })

    public Flux<TimeSeriesResult> getTestData(@RequestParam(required = false) Long start,
                                                 @RequestParam(required = false) Long end,
                                                 @RequestParam(required = false) Long interval) {

        final long endMs = end == null ? System.currentTimeMillis() : end;
        final long startMs = start == null ? (endMs - 300_000) : start;
        final long intervalMs = interval == null
                                    ? Math.max((endMs - startMs) / 1000L, 100L)
                                    : interval;
        return generate(startMs, endMs, intervalMs)
                .map(time -> new TimeSeriesResult(time, Math.floor(200 * Math.sin(time / 10_000.0)) / 10))
                .switchIfEmpty(notFound());
    }

    @GetMapping(value = "handshakes", produces = MediaType.APPLICATION_NDJSON_VALUE)
    public Flux<? extends HandshakeMessage> getHandshakes() {
        return messagingService.subscribeHandshake()
                .doOnNext(it -> log.info(": {}", it))
                .take(5);
    }

    @GetMapping(value = "uuid", produces = MediaType.TEXT_PLAIN_VALUE)
    public Flux<String> generateUUID(@RequestParam int range) {
        return generate(range);
    }

    @PutMapping(value = "power", produces = MediaType.TEXT_PLAIN_VALUE)
    public Mono<ResponseEntity<String>> setPower(@RequestParam String id, @RequestParam double power) {
        return messagingService.sendPower(id, power)
                .thenReturn(ResponseEntity.ok("OK"))
                .onErrorResume(IllegalArgumentException.class, th -> Mono.just(
                    ResponseEntity.status(HttpStatus.NOT_FOUND).body(th.getMessage()))
                )
                .onErrorResume(th -> Mono.just(ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(th.getMessage())));
    }

    @PutMapping(value = "speed", produces = MediaType.TEXT_PLAIN_VALUE)
    public Mono<ResponseEntity<String>> setSpeed(@RequestParam String id, @RequestParam double speed) {
        return trainService.setSpeed(id, speed)
                .thenReturn(ResponseEntity.ok("OK"))
                .onErrorResume(NotFoundException.class,
                        th -> Mono.just(ResponseEntity.status(HttpStatus.NOT_FOUND).body(th.getMessage())));
    }

    @SafeVarargs
    private static <T> Stream<T> concat(@NonNull Stream<T> ... streams) {
        if (streams.length == 0)
            return Stream.empty();

        Stream<T> result = streams[0];
        for (int i = 1; i < streams.length; i++)
            result = Stream.concat(result, streams[i]);

        return result;
    }

    @GetMapping(value = "transact", produces = MediaType.TEXT_PLAIN_VALUE)
    @Transactional(propagation = Propagation.REQUIRED)
    public Mono<String> transact() {
        return Mono.when(
                    concat(
                        Stream.of("1", "2", "3", "4", "5").map(k -> DBUtils.testTransact(k, connectionAccessor)),
                                //.map(m -> m.subscribeOn(Schedulers.boundedElastic()))
                        Stream.of(uuidGeneratorService.checkTransaction())
                    )
                .collect(Collectors.toList())
            )
            .thenReturn("OK");
    }

    private Flux<String> generate(int range) {
        return range <= 0 ? Flux.empty() : Flux.range(0, range).map(__ -> uuidGeneratorService.generate() + "\n");
    }
}
