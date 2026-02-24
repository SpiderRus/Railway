package org.spider.railway.utils;

import lombok.extern.slf4j.Slf4j;
import reactor.util.annotation.NonNull;

import java.time.Instant;
import java.time.OffsetDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeFormatterBuilder;
import java.util.ArrayList;
import java.util.List;

import static java.time.format.DateTimeFormatter.ISO_LOCAL_DATE_TIME;

@Slf4j
public class Utils {
    public static final ZoneId zoneId = ZoneId.systemDefault();

    public static final DateTimeFormatter odtFormatter = new DateTimeFormatterBuilder()
            .append(ISO_LOCAL_DATE_TIME)
            .optionalStart()
            .appendOffsetId()
            .toFormatter();

    private static final String[] intStringCache = new String[4096];

    static {
        for (int i = 0; i < intStringCache.length; i++)
            intStringCache[i] = Integer.toString(i);
    }

    @NonNull
    public static String toString(int value) {
        return (value & ~4096) == 0 ? intStringCache[value] : Integer.toString(value);
    }

    @NonNull
    public static String toString(long value) {
        return (value & ~4096) == 0 ? intStringCache[(int)value] : Long.toString(value);
    }

    public static short toUnsignedShort(byte x) {
        return (short) (((int) x) & 0xff);
    }

    public static long toUnsignedLong(int x) {
        return Integer.toUnsignedLong(x);
    }

    public static boolean isAfterOrEquals(@NonNull OffsetDateTime time1, @NonNull OffsetDateTime time2) {
        return time1.isAfter(time2) || time2.isEqual(time1);
    }

    public static boolean isBeforeOrEquals(@NonNull OffsetDateTime time1, @NonNull OffsetDateTime time2) {
        return time1.isBefore(time2) || time2.isEqual(time1);
    }

    @NonNull
    public static OffsetDateTime min(@NonNull OffsetDateTime time1, @NonNull OffsetDateTime time2) {
        return time1.isBefore(time2) ? time1 : time2;
    }

    @NonNull
    public static OffsetDateTime max(@NonNull OffsetDateTime time1, @NonNull OffsetDateTime time2) {
        return time1.isAfter(time2) ? time1 : time2;
    }

    @SafeVarargs
    @NonNull
    public static <T, L extends List<T>> L listOf(@NonNull L list, @NonNull T ... items) {
        for (T item : items)
            list.add(item);

        return list;
    }

    @SafeVarargs
    @NonNull
    public static <T> ArrayList<T> listOf(@NonNull T ... items) {
        return listOf(new ArrayList<>(items.length), items);
    }

    @NonNull
    public static OffsetDateTime ofMillis(long millis, @NonNull ZoneId zoneId) {
        return OffsetDateTime.ofInstant(Instant.ofEpochMilli(millis), zoneId);
    }

    @NonNull
    public static OffsetDateTime ofMillis(long millis) {
        return ofMillis(millis, zoneId);
    }

    public static double square(double value) {
        return value * value;
    }
}
