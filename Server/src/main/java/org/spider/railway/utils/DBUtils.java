package org.spider.railway.utils;

import io.r2dbc.spi.Connection;
import io.r2dbc.spi.R2dbcException;
import io.r2dbc.spi.Wrapped;
import lombok.extern.slf4j.Slf4j;
import org.springframework.r2dbc.connection.ConnectionHolder;
import org.springframework.r2dbc.core.ConnectionAccessor;
import org.springframework.transaction.NoTransactionException;
import org.springframework.transaction.reactive.TransactionContext;
import org.springframework.transaction.reactive.TransactionContextManager;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoSink;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuple4;
import reactor.util.function.Tuples;

import java.util.ArrayList;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.stream.Collectors;

@Slf4j
public class DBUtils {
    public interface TransactionKey {
        @NonNull
        <R> Mono<R> inConnection(@NonNull Function<Connection, Mono<R>> func);
    }

    private static class TransactionKeyWithConnection implements TransactionKey {
        private final Connection connection;

        public TransactionKeyWithConnection(@NonNull Connection connection) {
            this.connection = connection;
        }

        @Override
        public int hashCode() {
            return unwrap(connection).hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof TransactionKeyWithConnection otherKey))
                return false;

            return equals(otherKey);
        }

        public boolean equals(TransactionKeyWithConnection otherKey) {
            if (otherKey == this)
                return true;

            return unwrap(connection) == unwrap(otherKey.connection);
        }

        @Override
        public String toString() {
            return "" + connection;
        }

        @Override
        @NonNull
        public <R> Mono<R> inConnection(@NonNull Function<Connection, Mono<R>> func) {
            return func.apply(connection);
        }
    }

    private static class TransactionKeyWithoutConnection implements TransactionKey {
        private final ConnectionAccessor accessor;

        public TransactionKeyWithoutConnection(@NonNull ConnectionAccessor accessor) {
            this.accessor = accessor;
        }

        @Override
        public int hashCode() {
            return 0;
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof TransactionKeyWithoutConnection otherKey))
                return false;

            return equals(otherKey);
        }

        public boolean equals(TransactionKeyWithoutConnection otherKey) {
            return true;
        }

        @Override
        public String toString() {
            return "null";
        }

        @Override
        @NonNull
        public <R> Mono<R> inConnection(@NonNull Function<Connection, Mono<R>> func) {
            return accessor.inConnection(func::apply);
        }
    }


    @NonNull
    public static Mono<TransactionContext> getTransactionContext() {
        return Mono.deferContextual(ctx -> {
            final Object tcObject = ctx.getOrDefault(TransactionContext.class, null);

            if (tcObject == null) {
                log.warn("undefined transaction.");
                return Mono.empty();
            }

            if (!(tcObject instanceof TransactionContext transactionContext)) {
                log.warn("Transaction context is not a TransactionContext type ({})", tcObject.getClass().getName());
                return Mono.empty();
            }

            return Mono.justOrEmpty(transactionContext);
        });
    }

    @NonNull
    public static Mono<Object> getTransactionKey() {
        return getTransactionContext().flatMap(ctx -> {
                Object key = null;

                if (ctx != null) {
                    key = ctx.getResources().get("TRANSACTION-KEY");

                    if (key == null)
                        ctx.getResources().put("TRANSACTION-KEY", key = UUID.randomUUID().toString());
                }

                return Mono.justOrEmpty(key);
            });
    }

    @NonNull
    private static String printMap(Map<Object, Object> map) {
        if (map == null)
            return "{null}";

        return map.entrySet().stream().map(
                en -> en.getKey().getClass().getName() + " : " + en.getValue().getClass().getName()).collect(Collectors.joining(", "));
    }

    @Nullable
    private static Object extractTransactionKey(@NonNull TransactionContext transactionContext) {
        for (Object val : transactionContext.getResources().values())
            if (val instanceof ConnectionHolder connectionHolder) {
                return connectionHolder.getConnection().toString();
            }

        log.info("RES: {}", transactionContext.getResources());
        return null;
    }

//    @NonNull
//    public static TransactionKey getCurrentTransactionKey(@NonNull ContextView ctx) {
//        final Object tcObject = ctx.getOrDefault(TransactionContext.class, null);
//
//        if (tcObject == null) {
//            log.warn("undefined transaction.");
//            return NULL_KEY;
//        }
//
//        if (!(tcObject instanceof TransactionContext transactionContext)) {
//            log.warn("Transaction context is not a TransactionContext type ({})", tcObject.getClass().getName());
//            return NULL_KEY;
//        }
//
//        final Object key = extractTransactionKey(transactionContext);
//
//        // log.info("RES: {}, KEY: {}", printMap(transactionContext.getResources()), key);
//        return key == null ? NULL_KEY : new TransactionKey(key);
//    }
//
//    public static <R> Mono<R> inTransaction(Supplier<Mono<R>> func) {
//        return Mono.create(sink -> {
//            final ContextView ctx = sink.contextView();
//            final TransactionKey transactionKey = getCurrentTransactionKey(ctx);
//
//            Mono<R> result = func.get();
//
//            if (!transactionKey.isEmpty())
//                result = result.contextWrite(ctx);
//
//            result.subscribeOn(Schedulers.boundedElastic()).subscribe(sink::success, sink::error, sink::success);
//        });
//    }

    private static final LinkedBlockingQueue<Tuple4<MonoSink<String>, TransactionKey, Long, String>> testQueue = new LinkedBlockingQueue<>();

    private static <T> T unwrap(@NonNull T obj) {
        if (obj instanceof Wrapped<?> w) {
            T result = unwrap((T) w.unwrap());
            return result == null ? obj : result;
        }

        return obj;
    }

    private static Mono<String> getCurrentTransactionId(@NonNull Connection connection) {
        return Mono.from(connection
                        .createStatement("SELECT txid_current();")
                        // .createStatement("SELECT 1")
                        .execute())
                .flatMap(result -> Mono.from(result.map((row, __) -> "" + row.get(0))));
    }

    private static final Thread testExecutor = new Thread("transactionGrouper") {
        @Override
        public void run() {
            try {
                while (!Thread.currentThread().isInterrupted()) {
                    final var tt = testQueue.poll(1L, TimeUnit.SECONDS);

                    if (tt != null) {
                        final var map = Flux.<Tuple4<MonoSink<String>, TransactionKey, Long, String>>create(sink -> {
                            sink.next(tt);

                            Tuple4<MonoSink<String>, TransactionKey, Long, String> t;

                            while ((t = testQueue.poll()) != null)
                                sink.next(t);

                            sink.complete();
                        }).collectMultimap(Tuple2::getT2).block();

                        Mono.when(map.values().stream()
                                .map(tuple4s -> {
                                    return tuple4s.iterator().next().getT2()
                                            .inConnection(DBUtils::getCurrentTransactionId)
                                            .doOnSuccess(txId -> {
                                                for (var t : tuple4s) {
                                                    log.info("Success: {} ms", System.currentTimeMillis() - t.getT3());

                                                    t.getT1().success("Result-" + t.getT1());
                                                }
                                            })
                                            .onErrorResume(R2dbcException.class, th -> {
                                                for (var t : tuple4s)
                                                    t.getT1().error(th);

                                                return Mono.empty();
                                            })
                                            .subscribeOn(Schedulers.boundedElastic());
                                }).collect(Collectors.toList())).block();
                    }
                }
            } catch (InterruptedException e) {
                log.warn("Interrupt", e);
            }
        }
    };

    static {
        testExecutor.start();
    }

    private static Mono<String> addInQueue(@NonNull TransactionKey key, @NonNull String data) {
        return Mono.create(sink -> testQueue.add(Tuples.of(sink, key, System.currentTimeMillis(), data)));
    }

    @NonNull
    public static Mono<String> testTransact(@NonNull String key, @NonNull ConnectionAccessor connectionAccessor) {
        return Mono.defer(() -> {
            return TransactionContextManager.currentContext()
                    .flatMap(ctx -> {
                        if (ctx.isActualTransactionActive())
                            return connectionAccessor.inConnection(connection -> addInQueue(new TransactionKeyWithConnection(connection), key));

                        return addInQueue(new TransactionKeyWithoutConnection(connectionAccessor), key);
                    })
                    .onErrorResume(NoTransactionException.class, __ -> addInQueue(new TransactionKeyWithoutConnection(connectionAccessor), key));
        });
    }
}
