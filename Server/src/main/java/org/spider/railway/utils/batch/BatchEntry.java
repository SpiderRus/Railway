package org.spider.railway.utils.batch;

public class BatchEntry {
    volatile BatchEntry next;

    @SuppressWarnings("unchecked")
    public <E extends BatchEntry> E getNext() {
        return (E)next;
    }
}
