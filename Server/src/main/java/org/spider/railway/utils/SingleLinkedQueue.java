package org.spider.railway.utils;

import java.util.concurrent.atomic.AtomicStampedReference;

public class SingleLinkedQueue<T> {
    private static class Entry<T> {
        final T element;

        volatile Entry<T> next;

        Entry(T element) {
            this.element = element;
        }
    }

    private final AtomicStampedReference<Entry<T>> firstEntry = new AtomicStampedReference<>(null, 0);

    public void add(T value) {
        final Entry<T> entry = new Entry<>(value);
        Entry<T> first;
        int stamp;

        do {
            first = firstEntry.getReference();
            stamp = firstEntry.getStamp();

            entry.next = first;
        } while (!firstEntry.compareAndSet(first, entry, stamp, stamp + 1));
    }

    public T poll() {
        Entry<T> result;
        int stamp;

        do {
            if ((result = firstEntry.getReference()) == null)
                return null;

            stamp = firstEntry.getStamp();
        } while (!firstEntry.compareAndSet(result, result.next, stamp, stamp - 1));

        return result.element;
    }

    public int getSize() {
        return firstEntry.getStamp();
    }
}
