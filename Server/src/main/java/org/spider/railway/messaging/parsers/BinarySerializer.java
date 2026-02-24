package org.spider.railway.messaging.parsers;

import io.netty.buffer.ByteBuf;
import org.apache.commons.lang3.StringUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;

import java.lang.reflect.*;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.time.OffsetDateTime;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiFunction;
import java.util.function.Function;

public class BinarySerializer<T> {
    private static final Logger log = LoggerFactory.getLogger(BinarySerializer.class);

    private static final Map<Class<?>, Function<ByteBuf, Object>> SIMPLE_READERS = new HashMap<>();
    private static final Map<Class<?>, BiFunction<ByteBuf, Object, ByteBuf>> SIMPLE_WRITERS = new HashMap<>();

    static {
        SIMPLE_READERS.put(boolean.class, buf -> buf.readByte() != 0);
        SIMPLE_READERS.put(Boolean.class, buf -> buf.readByte() == 0 ? Boolean.FALSE : (buf.readByte() < 0 ? null : Boolean.TRUE));

        SIMPLE_READERS.put(byte.class, ByteBuf::readByte);
        SIMPLE_READERS.put(Byte.class, ByteBuf::readByte);

        SIMPLE_READERS.put(short.class, ByteBuf::readShortLE);
        SIMPLE_READERS.put(Short.class, ByteBuf::readShortLE);

        SIMPLE_READERS.put(Integer.class, ByteBuf::readIntLE);
        SIMPLE_READERS.put(int.class, ByteBuf::readIntLE);

        SIMPLE_READERS.put(Long.class, ByteBuf::readLongLE);
        SIMPLE_READERS.put(long.class, ByteBuf::readLongLE);

        SIMPLE_READERS.put(Date.class, buf -> {
            final long millis = buf.readLongLE();
            return millis <= 0 ? null : new Date(millis);
        });

        SIMPLE_WRITERS.put(boolean.class, (buf, obj) -> buf.writeByte(((boolean)obj) ? 1 : 0));
        SIMPLE_WRITERS.put(Boolean.class, (buf, obj) -> buf.writeByte(obj == null ? -1 : ((Boolean)obj) ? 1 : 0));

        SIMPLE_WRITERS.put(byte.class, (buf, obj) -> buf.writeByte((byte)obj));
        SIMPLE_WRITERS.put(Byte.class, (buf, obj) -> buf.writeByte(obj == null ? 0 : ((Byte)obj)));

        SIMPLE_WRITERS.put(short.class, (buf, obj) -> buf.writeShortLE((short)obj));
        SIMPLE_WRITERS.put(Short.class, (buf, obj) -> buf.writeShortLE(obj == null ? 0 : ((Short)obj)));

        SIMPLE_WRITERS.put(int.class, (buf, obj) -> buf.writeIntLE((int)obj));
        SIMPLE_WRITERS.put(Integer.class, (buf, obj) -> buf.writeIntLE(obj == null ? 0 : ((Integer)obj)));

        SIMPLE_WRITERS.put(long.class, (buf, obj) -> buf.writeLongLE((long)obj));
        SIMPLE_WRITERS.put(Long.class, (buf, obj) -> buf.writeLongLE(obj == null ? 0 : ((Long)obj)));

        SIMPLE_WRITERS.put(Date.class, (buf, obj) -> buf.writeLongLE(obj == null ? 0L : ((Date)obj).getTime()));
    }

    private static final ConcurrentHashMap<Class<?>, BinarySerializer<?>> serializers = new ConcurrentHashMap<>();

    private static class DeserializerInfo<T> {
        private static class SetterInfo {
            @NonNull
            final Function<ByteBuf, Object> reader;

            SetterInfo(@NonNull Function<ByteBuf, Object> reader) {
                this.reader = reader;
            }
        }

        private static class GetterInfo {
            @NonNull
            final Method method;

            @NonNull
            final BiFunction<ByteBuf, Object, ByteBuf> writer;

            GetterInfo(@NonNull Method method, @NonNull BiFunction<ByteBuf, Object, ByteBuf> writer) {
                this.method = method;
                this.writer = writer;
            }
        }

        @NonNull
        final Constructor<T> constructor;

        @NonNull
        final SetterInfo[] setters;

        @NonNull
        final GetterInfo[] getters;

        DeserializerInfo(@NonNull Constructor<T> constructor, @NonNull SetterInfo[] setters, @NonNull GetterInfo[] getters) {
            this.constructor = constructor;
            this.setters = setters;
            this.getters = getters;
        }
    }

    @NonNull
    private final DeserializerInfo<T> deserializerInfo;

    @NonNull
    private final Charset charset;

    @NonNull
    private final ZoneId zoneId;

    @NonNull
    private final Class<T> clazz;

    @NonNull
    @SuppressWarnings("unchecked")
    public static <T> BinarySerializer<T> get(@NonNull Class<T> clazz) {
        return (BinarySerializer<T>)serializers.computeIfAbsent(clazz, BinarySerializer::new);
    }

    private BinarySerializer(@NonNull Class<T> clazz) {
        this(clazz, StandardCharsets.UTF_8, ZoneId.systemDefault());
    }

    private BinarySerializer(@NonNull Class<T> clazz, @NonNull Charset charset, @NonNull ZoneId zoneId) {
        this.deserializerInfo = analyze(clazz);
        this.charset = charset;
        this.zoneId = zoneId;
        this.clazz = clazz;
    }

    @NonNull
    public T deserialize(@NonNull ByteBuf buffer) {
        try {
            Object[] params = new Object[deserializerInfo.setters.length];

            for (int i = 0; i < params.length; i++)
                params[i] = deserializerInfo.setters[i].reader.apply(buffer);

            return (T) deserializerInfo.constructor.newInstance(params);
        } catch (Throwable t) {
            throw new RuntimeException("Can't deserialize object for class " + clazz.getName(), t);
        }
    }

    @NonNull
    public ByteBuf serialize(@NonNull ByteBuf buffer, @NonNull T obj) {
        try {
            for (DeserializerInfo.GetterInfo getter : deserializerInfo.getters)
                buffer = getter.writer.apply(buffer, getter.method.invoke(obj));
        } catch (InvocationTargetException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }

        return buffer;
    }

    @NonNull
    private DeserializerInfo<T> analyze(Class<T> clazz) {
        log.info("Found serializer for class: {}", clazz);

        ArrayList<DeserializerInfo.SetterInfo> setters = new ArrayList<>();
        ArrayList<DeserializerInfo.GetterInfo> getters = new ArrayList<>();
        ArrayList<Class<?>> constructorArgs = new ArrayList<>();
        Constructor<T> constructor = null;

        for (Field field : clazz.getDeclaredFields()) {
            Function<ByteBuf, Object> reader = getReader(field.getType());

            if (reader != null) {
                String getterName = (field.getType().equals(boolean.class) || field.getType().equals(Boolean.class) ? "is" : "get") +
                            Character.toUpperCase(field.getName().charAt(0)) + field.getName().substring(1);
                try {
                    Method getterMethod = clazz.getDeclaredMethod(getterName);
                    BiFunction<ByteBuf, Object, ByteBuf> writer = getWriter(getterMethod.getReturnType());

                    if (writer == null)
                        throw new IllegalArgumentException("Writer for field " + field.getName() + " on class '" + clazz + "' not found");

                    getters.add(new DeserializerInfo.GetterInfo(getterMethod, writer));
                } catch (NoSuchMethodException e) {
                    throw new IllegalArgumentException("Getter method for field '" + field.getName() + ":" + field.getType().getName() +
                                    "' on class '" + clazz + "' not found");
                }

                constructorArgs.add(field.getType());
                setters.add(new DeserializerInfo.SetterInfo(reader));
            }
        }

        try {
            constructor = clazz.getDeclaredConstructor(constructorArgs.toArray(new Class[0]));
        } catch (NoSuchMethodException ignored) {}

        if (constructor == null)
            throw new IllegalArgumentException("Constructor not found for builder class " + clazz.getName());

        return new DeserializerInfo<>(constructor, setters.toArray(new DeserializerInfo.SetterInfo[0]),
                getters.toArray(new DeserializerInfo.GetterInfo[0]));
    }

    @Nullable
    private Function<ByteBuf, Object> getReader(@NonNull Class<?> type) {
        Function<ByteBuf, Object> reader = SIMPLE_READERS.get(type);

        if (reader == null) {
            if (type == String.class) {
                reader = buf -> {
                    short len = buf.readShortLE();
                    return len < 0 ? null : (len == 0 ? StringUtils.EMPTY : buf.readCharSequence(len, charset).toString());
                };
            } else if (type == ZonedDateTime.class) {
                reader = buf -> {
                    long millis = buf.readLongLE();
                    return millis <= 0 ? null : ZonedDateTime.ofInstant(Instant.ofEpochMilli(millis), zoneId);
                };
            } else if (type == OffsetDateTime.class) {
                reader = buf -> {
                    long millis = buf.readLongLE();
                    return millis <= 0 ? null : OffsetDateTime.ofInstant(Instant.ofEpochMilli(millis), zoneId);
                };
            } else if (type.isEnum()) {
                reader = buf -> {
                    short len = buf.readShortLE();
                    //noinspection unchecked,rawtypes
                    return len <= 0 ? null : Enum.valueOf((Class<? extends Enum>)type, buf.readCharSequence(len, charset).toString());
                };
            } else if (type.isArray()) {
                Class<?> arrayType = type.getComponentType();
                Function<ByteBuf, Object> nestedReader = getReader(arrayType);

                if (nestedReader != null) {
                    reader = buf -> {
                        short arrayLength = buf.readShortLE();

                        if (arrayLength < 0)
                            return null;

                        Object array = Array.newInstance(arrayType, arrayLength);

                        for (int i = 0; i < arrayLength; i++)
                            Array.set(array, i, nestedReader.apply(buf));

                        return array;
                    };
                }
            } else {
                final BinarySerializer<?> nestedSerializer = get(type);
                reader = nestedSerializer::deserialize;
            }
        }

        return reader;
    }

    @Nullable
    private BiFunction<ByteBuf, Object, ByteBuf> getWriter(@NonNull Class<?> type) {
        BiFunction<ByteBuf, Object, ByteBuf> writer = SIMPLE_WRITERS.get(type);

        if (writer == null) {
            if (type == String.class) {
                writer = (buf, obj) -> {
                    if (obj == null)
                        buf.writeShortLE(-1);
                    else {
                        int offset = buf.writerIndex();
                        buf.writeShortLE(0);
                        int len = buf.writeCharSequence((CharSequence) obj, charset);
                        if (len > 0)
                            buf.setShortLE(offset, len);
                    }
                    return buf;
                };
            } else if (type == ZonedDateTime.class) {
                writer = (buf, obj) -> buf.writeLongLE(obj == null ? 0L : ((ZonedDateTime)obj).toInstant().toEpochMilli());
            } else if (type == OffsetDateTime.class) {
                writer = (buf, obj) -> buf.writeLongLE(obj == null ? 0L : ((OffsetDateTime)obj).toInstant().toEpochMilli());
            } else if (type.isEnum()) {
                writer = (buf, obj) -> {
                    if (obj == null)
                        buf.writeShortLE(-1);
                    else {
                        int offset = buf.writerIndex();
                        buf.writeShortLE(0);
                        int len = buf.writeCharSequence(((Enum<?>)obj).name(), charset);
                        if (len > 0)
                            buf.setShortLE(offset, len);
                    }
                    return buf;
                };
            } else if (type.isArray()) {
                Class<?> arrayType = type.getComponentType();
                BiFunction<ByteBuf, Object, ByteBuf> nestedWriter = getWriter(arrayType);

                if (nestedWriter != null) {
                    writer = (buf, obj) -> {
                        if (obj == null)
                            buf.writeShortLE(-1);
                        else {
                            short arrayLength = (short)Array.getLength(obj);
                            buf.writeShortLE(arrayLength);

                            for (int i = 0; i < arrayLength; i++)
                                buf = nestedWriter.apply(buf, Array.get(obj, i));
                        }
                        return buf;
                    };
                }
            } else {
                //noinspection unchecked
                final BinarySerializer<Object> nestedSerializer = (BinarySerializer<Object>)get(type);

                writer = nestedSerializer::serialize;
            }
        }

        return writer;
    }
}
