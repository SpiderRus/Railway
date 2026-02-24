package org.spider.railway.utils.batch;

import reactor.util.annotation.NonNull;

import java.util.Iterator;
import java.util.concurrent.atomic.AtomicReferenceFieldUpdater;
import java.util.function.Consumer;
import java.util.function.Function;

public abstract class BatchProcessor<T extends BatchEntry> {
    private static final int INITIAL_ARRAY_SIZE = 128;
    private static final int MAX_DEPTH = 10_000;

    private static final AtomicReferenceFieldUpdater<BatchProcessor, BatchEntry> firstEntryUpdater =
            AtomicReferenceFieldUpdater.newUpdater(BatchProcessor.class, BatchEntry.class, "firstEntry");

    private volatile BatchEntry firstEntry;

    protected final T put(T entry) {
        BatchEntry first;

        //noinspection StatementWithEmptyBody
        while (!firstEntryUpdater.compareAndSet(this, entry.next = first = firstEntry, entry))
            ;

        if (first == null)
            processChain(null);

        return entry;
    }

    @SuppressWarnings("unchecked")
    private void processChain(T last) {
        process((T)firstEntry, last);
    }

    protected final void doFinal(T first) {
        if (!firstEntryUpdater.compareAndSet(this, first, null)) {
            first.next = null;

            processChain(first);
        }
    }

    protected void process(T first, T last) {
        doFinal(first);
    }

    protected interface Collection<T> extends Iterable<T> {
        <R> Iterator<R> map(Function<T, R> mapper);
    }

    private static class CollectionImpl<T> implements Collection<T> {
        private final Object[] array;
        private final int offset;

        CollectionImpl(Object[] array, int offset) {
            this.array = array;
            this.offset = offset;
        }

        @Override
        @NonNull
        public Iterator<T> iterator() {
            return new Iterator<T>() {
                int offset = CollectionImpl.this.offset;

                @Override
                public boolean hasNext() {
                    return offset < array.length;
                }

                @Override
                @SuppressWarnings("unchecked")
                public T next() {
                    return (T)array[offset++];
                }
            };
        }

        public <R> Iterator<R> map(Function<T, R> mapper) {
            return new Iterator<R>() {
                int offset = CollectionImpl.this.offset;

                @Override
                public boolean hasNext() {
                    return offset < array.length;
                }

                @Override
                @SuppressWarnings("unchecked")
                public R next() {
                    return mapper.apply((T)array[offset++]);
                }
            };
        }
    }

    private static class SingletonIterator<T> implements Iterator<T> {
        private T obj;

        SingletonIterator(T obj) {
            this.obj = obj;
        }

        @Override
        public boolean hasNext() {
            return obj != null;
        }

        @Override
        public T next() {
            final T obj = this.obj;
            this.obj = null;
            return obj;
        }
    };

    private static class SingletonCollectionImpl<T> implements Collection<T> {
        private final T value;

        SingletonCollectionImpl(T value) {
            this.value = value;
        }

        @Override
        public Iterator<T> iterator() {
            return new SingletonIterator<>(value);
        }

        public <R> Iterator<R> map(Function<T, R> mapper) {
            return new SingletonIterator<>(mapper.apply(value));
        }
    }

    private static class ForEachContainer<T> {
        int counter = 0;
        final T last;
        final Consumer<T> consumer;

        ForEachContainer(T last, Consumer<T> consumer) {
            this.last = last;
            this.consumer = consumer;
        }
    }

    protected static <T extends BatchEntry> Collection<T> asIterable(T first, T last) {
        if (first.next == last)
            return new SingletonCollectionImpl<>(first);

        Object[] array = new Object[INITIAL_ARRAY_SIZE];

        int offset = array.length;
        for (T i = first; i != last; i = i.getNext()) {
            if (offset == 0) {
                Object[] oldArr = array;
                offset = oldArr.length >>> 1;

                array = new Object[oldArr.length + offset];

                System.arraycopy(oldArr, 0, array, offset, oldArr.length);
            }

            array[--offset] = i;
        }

        return new CollectionImpl<>(array, offset);
    }

    protected static <T extends BatchEntry, R> Iterable<R> asIterable(T first, T last, Function<T, R> mapper) {
        if (first.next == last)
            return new SingletonCollectionImpl<>(mapper.apply(first));

        Object[] array = new Object[INITIAL_ARRAY_SIZE];

        int offset = array.length;
        for (T i = first; i != last; i = i.getNext()) {
            if (offset == 0) {
                Object[] oldArr = array;
                offset = oldArr.length >>> 1;

                array = new Object[oldArr.length + offset];

                System.arraycopy(oldArr, 0, array, offset, oldArr.length);
            }

            array[--offset] = mapper.apply(i);
        }

        return new CollectionImpl<>(array, offset);
    }

    protected static <T extends BatchEntry> Iterator<T> asIterator(T first, T last) {
        return asIterator(first, last, INITIAL_ARRAY_SIZE);
    }

    private static <T extends BatchEntry> Iterator<T> asIterator(T first, T last, int arraySize) {
        if (first.next == last)
            return new SingletonIterator<>(first);

        Object[] array = new Object[arraySize];

        int offset = array.length;

        for (T i = first; i != last; i = i.getNext()) {
            if (offset == 0) {
                Object[] oldArr = array;
                offset = oldArr.length >>> 1;

                array = new Object[oldArr.length + offset];

                System.arraycopy(oldArr, 0, array, offset, oldArr.length);
            }

            array[--offset] = i;
        }

        final int f = offset;
        final Object[] arr = array;
        return new Iterator<T>() {
            int off = f;

            @Override
            public boolean hasNext() {
                return off < arr.length;
            }

            @Override
            @SuppressWarnings("unchecked")
            public T next() {
                return (T)arr[off++];
            }
        };
    }

    protected static <T extends BatchEntry, R> Iterator<R> asIterator(T first, T last, Function<T, R> mapper) {
        if (first.next == last)
            return new SingletonIterator<>(mapper.apply(first));

        Object[] array = new Object[INITIAL_ARRAY_SIZE];

        int offset = array.length;

        for (T i = first; i != last; i = i.getNext()) {
            if (offset == 0) {
                Object[] oldArr = array;
                offset = oldArr.length >>> 1;

                array = new Object[oldArr.length + offset];

                System.arraycopy(oldArr, 0, array, offset, oldArr.length);
            }

            array[--offset] = mapper.apply(i);
        }

        final int f = offset;
        final Object[] arr = array;
        return new Iterator<R>() {
            int off = f;

            @Override
            public boolean hasNext() {
                return off < arr.length;
            }

            @Override
            @SuppressWarnings("unchecked")
            public R next() {
                return mapper.apply((T)arr[off++]);
            }
        };
    }

    @SuppressWarnings("unchecked")
    private static <T extends BatchEntry> boolean forEach(T en, ForEachContainer<T> container) {
        if (en != container.last) {
            if (++container.counter >= MAX_DEPTH)
                return true;

            if (forEach((T) en.next, container))
                return true;

            container.consumer.accept(en);
        }

        return false;
    }

    protected static <T extends BatchEntry> void forEach(T first, T last, Consumer<T> consumer) {
        if (first.next == last)
            consumer.accept(first);
        else
            if (forEach(first, new ForEachContainer<>(last, consumer)))
                for (Iterator<T> it = asIterator(first, last, 2 * MAX_DEPTH); it.hasNext();)
                    consumer.accept(it.next());
    }
}
