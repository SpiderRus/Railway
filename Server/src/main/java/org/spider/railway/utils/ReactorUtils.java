package org.spider.railway.utils;

import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxSink;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoSink;
import reactor.core.scheduler.Scheduler;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.function.Tuple2;

import java.util.HashMap;
import java.util.function.BiPredicate;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Stream;

public class ReactorUtils {
    private static final Predicate<?> TRUE_PREDICATE = __ -> true;

    @NonNull
    public static <T> Flux<T> publishSingle(Flux<T> src) {
        return Flux.using(
                () -> Schedulers.single(Schedulers.boundedElastic()),
                src::publishOn,
                Scheduler::dispose
            );
    }

    @NonNull
    @SuppressWarnings("unchecked")
    public static <T> Predicate<T> truePredicate() {
        return (Predicate<T>) TRUE_PREDICATE;
    }

    @NonNull
    public static Mono<Void> when(@NonNull Stream<Mono<?>> monos) {
        return Mono.when(Flux.fromStream(monos));
    }

    @NonNull
    public static Mono<Void> whenAsync(@NonNull Stream<Mono<?>> monos) {
        final Scheduler scheduler = Schedulers.boundedElastic();
        return Mono.when(Flux.fromStream(monos.map(it -> it.subscribeOn(scheduler))));
    }

    @NonNull
    public static <T> Flux<T> merge(@NonNull Stream<Flux<T>> fluxes) {
        return Flux.merge(Flux.fromStream(fluxes));
    }

    public static <T> void schedule(@NonNull Scheduler scheduler, @NonNull Iterable<T> objects, @NonNull Function<T, Runnable> function) {
        for (T object : objects)
            scheduler.schedule(function.apply(object));
    }

    public static <T> void schedule(@NonNull Iterable<T> objects, @NonNull Function<T, Runnable> function) {
        schedule(Schedulers.boundedElastic(), objects, function);
    }

    public static void error(@NonNull Stream<MonoSink<?>> sinks, @NonNull Throwable throwable) {
        final Scheduler scheduler = Schedulers.boundedElastic();

        sinks.forEach(sink -> scheduler.schedule(() -> sink.error(throwable)));
    }

    public static <T, R> void error(@NonNull Iterable<Tuple2<T, MonoSink<R>>> tuples, @NonNull Throwable throwable) {
        final Scheduler scheduler = Schedulers.boundedElastic();

        for (Tuple2<T, MonoSink<R>> t : tuples)
            scheduler.schedule(() -> t.getT2().error(throwable));
    }

    public static <T, R> Mono<Void> handleResult(Flux<R> result, @NonNull Iterable<Tuple2<T, FluxSink<R>>> tuples,
                                                 @NonNull BiPredicate<T, R> filter) {
        return result
                .doOnNext(item -> {
                    for (var tuple : tuples)
                        if (filter.test(tuple.getT1(), item))
                            tuple.getT2().next(item);
                })
                .then()
                .doOnSuccess(__ -> {
                    for (var tuple : tuples)
                        tuple.getT2().complete();
                })
                .onErrorResume(th -> {
                    for (var tuple : tuples)
                        tuple.getT2().error(th);
                    return Mono.empty();
                }).subscribeOn(Schedulers.boundedElastic());
    }
}
