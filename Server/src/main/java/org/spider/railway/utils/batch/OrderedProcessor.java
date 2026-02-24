package org.spider.railway.utils.batch;

import reactor.core.publisher.Mono;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuples;

import java.util.function.Function;

public class OrderedProcessor<T, R> extends BatchProcessor<OrderedProcessor.Entry<T, R>> {
    private static final Object NULL_OBJECT = new Object();

    protected static class Entry<T, R> extends BatchEntry {
        final Tuple2<T, R> data;

        Entry(T input, R output) {
            this.data = Tuples.of(input, output);
        }
    }

    private final Function<Iterable<Tuple2<T, R>>, Mono<Void>> processor;

    private final Function<Entry<T, R>, Tuple2<T, R>> mapper = en -> en.data;

    public OrderedProcessor(Function<Iterable<Tuple2<T, R>>, Mono<Void>> processor) {
        this.processor = processor;
    }

    @Override
    protected void process(Entry<T, R> first, Entry<T, R> last) {
        processor.apply(asIterable(first, last, mapper))
                .doFinally(__ -> doFinal(first))
                .subscribe();
    }

    public R process(T value, R result) {
        put(new Entry<>(value, result));

        return result;
    }

    @SuppressWarnings("unchecked")
    public void process(T value) {
        process(value, (R) NULL_OBJECT);
    }

    @SuppressWarnings("unchecked")
    public void process() {
        process((T) NULL_OBJECT, (R) NULL_OBJECT);
    }
}
