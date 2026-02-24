package org.spider.railway.messaging;

import io.netty.buffer.ByteBuf;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.SerializationException;
import org.spider.railway.exception.NotFoundException;
import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.message.*;
import org.spider.railway.messaging.parsers.BinarySerializer;
import org.spider.railway.utils.ReactorUtils;
import org.spider.railway.utils.Utils;
import org.spider.railway.utils.udp.UdpClient;
import org.spider.railway.utils.udp.UdpServer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Primary;
import org.springframework.data.repository.reactive.ReactiveCrudRepository;
import org.springframework.stereotype.Service;
import reactor.core.Disposable;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Scheduler;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;
import reactor.util.function.Tuple3;

import java.net.InetSocketAddress;
import java.time.OffsetDateTime;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Stack;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicStampedReference;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import static org.spider.railway.messaging.parsers.ParserUtils.parsePackets;

@Slf4j
@Service
@Primary
public class UdpMessagingServiceImpl implements MessagingService {
    private static final short TRAIN_CODE = 0;

    private static final short SWITCH_CODE = 1;

    private static final short SEMAPHORE_CODE = 2;

    private static final short SWITCH_WITH_SEMAPHORE_CODE = 3;

    private static final String SUBITEMS_DELIMETER = "-";

    @NonNull
    private static String createClientId(@NonNull String clientId) {
        return clientId.intern();
    }

    @NonNull
    private static String createSemaphoreId(@NonNull String clientId, int index) {
        return createClientId(clientId + SUBITEMS_DELIMETER + index);
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class PowerEntity {
        private final byte type;
        private final int power;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SpeedEntity {
        private final byte type;
        private final int speed;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SwitchEntity {
        private final byte type;
        private final byte on;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SemaphoreEntity {
        private final byte type;
        private final short index;
        private final int color;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class HandshakeEntity {
        private final String deviceId;
        private final short typeCode;
        private final OffsetDateTime timestamp;
        private final byte subItems;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class LogEntity {
        private final String deviceId;
        private final OffsetDateTime timestamp;
        private final byte core;
        private final byte level;
        private final String tag;
        private final String message;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class TrainStatusEntity {
        private final OffsetDateTime timestamp;
        private final int version;
        private final int internalTemp;
        private final short speed; // mm/sec
        private final int power; // 1/1000 percent
        private final OffsetDateTime schemeMarkerTime;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SwitchStatusEntity {
        private final OffsetDateTime timestamp;
        private final int version;
        private final int internalTemp;
        private final boolean on;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SemaphoreStatusEntity {
        private final OffsetDateTime timestamp;
        private final int version;
        private final int internalTemp;
        private final int color;
    }

    @Builder
    @Getter
    @ToString
    @AllArgsConstructor
    public static class SwitchWithSemaphoresStatusEntity {
        private final OffsetDateTime timestamp;
        private final int version;
        private final int internalTemp;
        private final boolean state;
        private final int[] semaphoreColors;
    }

    @ToString
    private static class HandshakeMessageImpl extends AbstractMessage implements HandshakeMessage {
        private final SchemeItem.Type type;

        HandshakeMessageImpl(@NonNull String id, @NonNull SchemeItem.Type type) {
            super(id);

            this.type = type;
        }

        @Override
        @NonNull
        public SchemeItem.Type getType() {
            return this.type;
        }
    }

    @ToString
    private static class LogMessageImpl extends AbstractMessage implements LogMessage {
        private final InetSocketAddress address;
        private final LogEntity logEntity;

        LogMessageImpl(@NonNull String id, @NonNull InetSocketAddress address, @NonNull LogEntity logEntity) {
            super(id);

            this.logEntity = logEntity;
            this.address = address;
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return logEntity.getTimestamp();
        }

        @Override
        public int getCore() {
            return logEntity.getCore();
        }

        @Override
        public int getLevel() {
            return logEntity.getLevel();
        }

        @Override
        @NonNull
        public String getTag() {
            return logEntity.getTag();
        }

        @Override
        @NonNull
        public String getMessage() {
            return logEntity.getMessage();
        }

        @Override
        @NonNull
        public InetSocketAddress getAddress() {
            return this.address;
        }
    }

    @Getter
    private static class ClientInfo {
        private final String id;
        private final short clientTypeCode;
        private final InetSocketAddress address;
        private final UdpClient udpClient;
        private final int subItems;
        private final AtomicLong version = new AtomicLong();
        private final AtomicStampedReference<Long> correctionMillis = new AtomicStampedReference<>(0L, 0);

        ClientInfo(String id, short clientTypeCode, InetSocketAddress address, int subItems) {
            this.id = id;
            this.clientTypeCode = clientTypeCode;
            this.address = address;
            this.udpClient = new UdpClient(address);
            this.subItems = subItems;
        }

        public void setVersion(long newVersion) {
            long oldVersion;

            do {
                if ((oldVersion = this.version.get()) >= newVersion)
                    break;
            } while (!this.version.compareAndSet(oldVersion, newVersion));
        }

        @NonNull
        public OffsetDateTime correctTimestamp(@NonNull OffsetDateTime fromItem, @NonNull OffsetDateTime received) {
            final long millis = received.toInstant().toEpochMilli() - fromItem.toInstant().toEpochMilli();

            int oldNumber, newNumber;
            Long oldSum;
            long newSum;
            
            do {
                oldNumber = correctionMillis.getStamp();
                oldSum = correctionMillis.getReference();
            } while (!correctionMillis.compareAndSet(oldSum, newSum = oldSum + millis, oldNumber, newNumber = oldNumber + 1));

            final OffsetDateTime result = fromItem.plus(newSum / newNumber, ChronoUnit.MILLIS);

            return result.isAfter(received) ? received : result;
        }
    }

    @ToString
    private static class TrainStatusMessageImpl extends AbstractMessage implements TrainStatusMessage {
        private final TrainStatusEntity entity;
        private final OffsetDateTime correctedTimestamp;

        protected TrainStatusMessageImpl(@NonNull String id, @NonNull TrainStatusEntity entity, @NonNull ClientInfo client) {
            super(id);

            this.entity = entity;
            this.correctedTimestamp = client.correctTimestamp(entity.getTimestamp(), getReceiveTimestamp());
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp() / 1000.0;
        }

        @Override
        public long getVersion() {
            return entity.getVersion();
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return correctedTimestamp;
        }

        @Override
        public int getSpeed() {
            return entity.getSpeed();
        }

        @Override
        public double getPower() {
            return entity.getPower() / 1000.0;
        }

        @Override
        @Nullable
        public OffsetDateTime getSchemeMarkerTime() {
            return entity.getSchemeMarkerTime();
        }
    }

    @ToString
    private static class SwitchStatusMessageImpl extends AbstractMessage implements SwitchStatusMessage {
        private final SwitchStatusEntity entity;
        private final OffsetDateTime correctedTimestamp;

        protected SwitchStatusMessageImpl(@NonNull String id, @NonNull SwitchStatusEntity entity, @NonNull ClientInfo client) {
            super(id);

            this.entity = entity;
            this.correctedTimestamp = client.correctTimestamp(entity.getTimestamp(), getReceiveTimestamp());
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        public long getVersion() {
            return Utils.toUnsignedLong(entity.getVersion());
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return correctedTimestamp;
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp() / 1000.0;
        }

        @Override
        public boolean getState() {
            return entity.isOn();
        }
    }

    @ToString
    private static class SwitchStatusMessageMImpl extends AbstractMessage implements SwitchStatusMessage {
        private final SwitchWithSemaphoresStatusEntity entity;
        private final OffsetDateTime correctedTimestamp;

        protected SwitchStatusMessageMImpl(@NonNull String id, @NonNull SwitchWithSemaphoresStatusEntity entity, @NonNull ClientInfo client) {
            super(id);

            this.entity = entity;
            this.correctedTimestamp = client.correctTimestamp(entity.getTimestamp(), getReceiveTimestamp());
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        public long getVersion() {
            return Utils.toUnsignedLong(entity.getVersion());
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return correctedTimestamp;
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp() / 1000.0;
        }

        @Override
        public boolean getState() {
            return entity.isState();
        }
    }

    @ToString
    private static class SemaphoreStatusMessageImpl extends AbstractMessage implements SemaphoreStatusMessage {
        private final SemaphoreStatusEntity entity;
        private final OffsetDateTime correctedTimestamp;

        protected SemaphoreStatusMessageImpl(@NonNull String id, @NonNull SemaphoreStatusEntity entity, @NonNull ClientInfo client) {
            super(id);

            this.entity = entity;
            this.correctedTimestamp = client.correctTimestamp(entity.getTimestamp(), getReceiveTimestamp());
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        public long getVersion() {
            return Utils.toUnsignedLong(entity.getVersion());
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return correctedTimestamp;
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp() / 1000.0;
        }

        @Override
        public int getColor() {
            return entity.getColor();
        }
    }

    @ToString
    private static class SemaphoreStatusMessageMImpl extends AbstractMessage implements SemaphoreStatusMessage {
        private final SwitchWithSemaphoresStatusEntity entity;
        private final int index;
        private final OffsetDateTime correctedTimestamp;

        protected SemaphoreStatusMessageMImpl(@NonNull String id, @NonNull SwitchWithSemaphoresStatusEntity entity,
                                              int index, @NonNull ClientInfo client) {
            super(id);

            this.entity = entity;
            this.index = index;
            this.correctedTimestamp = client.correctTimestamp(entity.getTimestamp(), getReceiveTimestamp());
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        public long getVersion() {
            return Utils.toUnsignedLong(entity.getVersion());
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return correctedTimestamp;
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp() / 1000.0;
        }

        @Override
        public int getColor() {
            return entity.getSemaphoreColors()[index];
        }
    }

    private static final BinarySerializer<HandshakeEntity> handshakeSerializer = BinarySerializer.get(HandshakeEntity.class);

    private static final BinarySerializer<LogEntity> logSerializer = BinarySerializer.get(LogEntity.class);

    private static final BinarySerializer<TrainStatusEntity> trainStatusSerializer = BinarySerializer.get(TrainStatusEntity.class);

    private static final BinarySerializer<SwitchStatusEntity> switchStatusSerializer = BinarySerializer.get(SwitchStatusEntity.class);

    private static final BinarySerializer<SemaphoreStatusEntity> semaphoreStatusSerializer = BinarySerializer.get(SemaphoreStatusEntity.class);

    private static final BinarySerializer<SwitchWithSemaphoresStatusEntity> switchWithSemaphoresStatusSerializer =
            BinarySerializer.get(SwitchWithSemaphoresStatusEntity.class);

    private final UdpServer udpServer;

    private final Flux<? extends HandshakeMessage> handshakesSrc;

    private final Flux<? extends LogMessage> logsSrc;

    private final Flux<? extends StatusMessage> statusSrc;

    private final ConcurrentHashMap<String, ClientInfo> clientById = new ConcurrentHashMap<>();

    private final ConcurrentHashMap<InetSocketAddress, ClientInfo> clientByAddress = new ConcurrentHashMap<>();

    private Disposable handshakes;

    private Disposable statuses;

    @Autowired
    public UdpMessagingServiceImpl(@Value("${messaging.udp.port}") int port) {
        this.udpServer = new UdpServer(port);
        final Flux<Tuple3<Byte, InetSocketAddress, ByteBuf>> src = parsePackets(udpServer.subscribe());

        this.handshakesSrc = src.concatMap(t -> {
                try {
                    if (t.getT1() != 2)
                        return Mono.empty();
                    final OffsetDateTime now = OffsetDateTime.now();

                    final HandshakeEntity handshakeEntity = handshakeSerializer.deserialize(t.getT3().duplicate());
                    log.info("Handshake: address: {}, {}", t.getT2(), handshakeEntity);

                    final String clientId = createClientId(handshakeEntity.getDeviceId());
                    final List<? extends HandshakeMessage> result = processHandshakeMessage(clientId, handshakeEntity);

                    if (result == null)
                        return Mono.empty();

                    ClientInfo client = clientById.get(clientId);
                    if (client == null
                            || !client.getAddress().equals(t.getT2())
                            || client.getClientTypeCode() != handshakeEntity.getTypeCode()) {
                        if (client != null)
                            clientByAddress.remove(client.getAddress());

                        client = new ClientInfo(clientId, handshakeEntity.getTypeCode(), t.getT2(), result.size());
                        clientByAddress.put(client.getAddress(), client);
                        for (HandshakeMessage message : result)
                            clientById.put(message.getId(), client);
                    }

                    client.correctTimestamp(handshakeEntity.getTimestamp(), now);
                    client.getUdpClient().send(buf -> {
                                if (buf.writableBytes() < 1)
                                    return false;
                                buf.writeByte(3);
                                return true;
                            })
                            .subscribeOn(Schedulers.boundedElastic())
                            .doOnError(th -> log.error("send()", th))
                            .subscribe();

                    return Flux.fromIterable(result);
                } finally {
                    t.getT3().release();
                }
            })
            .share();

        this.logsSrc = src.concatMap(t -> {
                    try {
                        if (t.getT1() != 0)
                            return Mono.empty();

                        final ClientInfo client = clientByAddress.get(t.getT2());

                        if (client == null) {
                            log.warn("Log from unknown client addr={}.", t.getT2());

                            return Mono.empty();
                        }
                        final LogEntity logEntity = logSerializer.deserialize(t.getT3().duplicate());

                        return Mono.just(new LogMessageImpl(client.getId(), client.getAddress(), logEntity));
                    } finally {
                        t.getT3().release();
                    }
                })
                .share();

        this.statusSrc = src.concatMap(t -> {
                    try {
                        if (t.getT1() == 1) {
                            final ClientInfo client = clientByAddress.get(t.getT2());

                            if (client != null) {
                                List<? extends StatusMessage> result = processStatusMessage(client, t.getT3());

                                if (result != null) {
                                    if (!result.isEmpty())
                                        client.setVersion(result.get(0).getVersion());

                                    return Flux.fromIterable(result);
                                }

                                log.warn("Unprocessed status message. address={}.", t.getT2());
                            } else
                                log.warn("Status from unknown client addr={}.", t.getT2());
                        }

                        return Mono.empty();
                    } finally {
                        t.getT3().release();
                    }
                })
                .share();
    }

    @Nullable
    private List<? extends HandshakeMessage> processHandshakeMessage(@NonNull String clientId, @NonNull HandshakeEntity handshakeEntity) {
        switch (handshakeEntity.getTypeCode()) {
            case TRAIN_CODE:
                return List.of(new HandshakeMessageImpl(clientId, SchemeItem.Type.TRAIN));

            case SWITCH_CODE:
                return List.of(new HandshakeMessageImpl(clientId, SchemeItem.Type.SWITCH));

            case SEMAPHORE_CODE:
                return List.of(new HandshakeMessageImpl(clientId, SchemeItem.Type.SEMAPHORE));

            case SWITCH_WITH_SEMAPHORE_CODE:
                final ArrayList<HandshakeMessageImpl> messages = new ArrayList<>(handshakeEntity.getSubItems() + 1);
                messages.add(new HandshakeMessageImpl(clientId, SchemeItem.Type.SWITCH));
                for (int i = 0; i < handshakeEntity.subItems; i++)
                    messages.add(new HandshakeMessageImpl(createSemaphoreId(clientId, i), SchemeItem.Type.SEMAPHORE));
                return messages;

            default:
                log.warn("Unknown type={} for id={}", handshakeEntity.getTypeCode(), clientId);

                return null;
        }
    }

    @Nullable
    private List<? extends StatusMessage> processStatusMessage(ClientInfo client, ByteBuf buf) {
        switch (client.getClientTypeCode()) {
            case TRAIN_CODE:
                final TrainStatusEntity trainStatusEntity = trainStatusSerializer.deserialize(buf.duplicate());
                log.info("Receive status from {}: {}", client.getId(), trainStatusEntity);

                return List.of(new TrainStatusMessageImpl(client.getId(), trainStatusEntity, client));

            case SWITCH_CODE:
                final SwitchStatusEntity switchStatusEntity = switchStatusSerializer.deserialize(buf.duplicate());
                log.info("Receive status from {}: {}", client.getId(), switchStatusEntity);

                return List.of(new SwitchStatusMessageImpl(client.getId(), switchStatusEntity, client));

            case SEMAPHORE_CODE:
                final SemaphoreStatusEntity semaphoreStatusEntity = semaphoreStatusSerializer.deserialize(buf.duplicate());
                log.info("Receive status from {}: {}", client.getId(), semaphoreStatusEntity);

                return List.of(new SemaphoreStatusMessageImpl(client.getId(), semaphoreStatusEntity, client));

            case SWITCH_WITH_SEMAPHORE_CODE:
                final SwitchWithSemaphoresStatusEntity switchWithSemaphoresStatusEntity =
                        switchWithSemaphoresStatusSerializer.deserialize(buf.duplicate());
                log.info("Receive status from {}: {}", client.getId(), switchWithSemaphoresStatusEntity);

                final int numSemaphores = switchWithSemaphoresStatusEntity.getSemaphoreColors().length;
                final ArrayList<StatusMessage> result = new ArrayList<>(numSemaphores + 1);
                result.add(new SwitchStatusMessageMImpl(client.getId(), switchWithSemaphoresStatusEntity, client));
                for (int i = 0; i < numSemaphores; i++)
                    result.add(new SemaphoreStatusMessageMImpl(createSemaphoreId(client.getId(), i),
                                switchWithSemaphoresStatusEntity, i, client));
                return result;

            default:
                return null;
        }
    }

    @PostConstruct
    public void init() {
        udpServer.start();

        handshakes = subscribeHandshake().subscribe();

        statuses = subscribeStatus().subscribe();
    }

    @PreDestroy
    public void destroy() {
        handshakes.dispose();

        statuses.dispose();

        udpServer.destroy();
    }

    @Override
    @NonNull
    public Flux<? extends HandshakeMessage> subscribeHandshake() {
        return ReactorUtils.publishSingle(handshakesSrc);
    }

    @Override
    @NonNull
    public Flux<? extends LogMessage> subscribeLog() {
        return ReactorUtils.publishSingle(logsSrc);
    }

    @Override
    @NonNull
    public Flux<? extends StatusMessage> subscribeStatus() {
        return ReactorUtils.publishSingle(statusSrc);
    }

    private Flux<? extends StatusMessage> subscribeStatusInternal(@NonNull SchemeItem.Type type,
                                                                  @NonNull Predicate<StatusMessage> predicate) {
        return statusSrc.filter(msg -> msg.getItemType() == type && predicate.test(msg));
    }

    private Flux<? extends StatusMessage> subscribeStatusInternal(@NonNull Predicate<StatusMessage> predicate) {
        return statusSrc.filter(predicate);
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull SchemeItem.Type type) {
        //noinspection unchecked
        return ReactorUtils.publishSingle(subscribeStatusInternal(type, ReactorUtils.truePredicate())).map(msg -> (T)msg);
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType) {
        return ReactorUtils.publishSingle(subscribeStatusInternal(message -> messageType.isAssignableFrom(message.getClass())))
                .cast(messageType);
    }

    @Override
    @NonNull
    public <T extends StatusMessage> Flux<T> subscribeStatus(@NonNull Class<T> messageType, @NonNull Scheduler scheduler) {
        return subscribeStatusInternal(message -> messageType.isAssignableFrom(message.getClass()))
                .cast(messageType)
                .publishOn(scheduler);
    }

    @NonNull
    private <T> Mono<Void> send(@NonNull ClientInfo client, @NonNull T object) {
        return Mono.defer(() -> {
                final BinarySerializer<T> serializer;

                try {
                    //noinspection unchecked
                    serializer = BinarySerializer.get((Class<T>) object.getClass());
                } catch (Throwable t) {
                    return Mono.error(new SerializationException("Can't get serializer object {" + object + "} for class: " + object.getClass(), t));
                }

                return client.getUdpClient().send(buf -> {
                    final int writerIndex = buf.writerIndex();
                    try {
                        serializer.serialize(buf, object);
                    } catch (IndexOutOfBoundsException e) {
                        return false;
                    } catch (Throwable t) {
                        log.error("Can't serialize object {" + object + "} for class: " + object.getClass(), t);

                        buf.writerIndex(writerIndex);
                    }

                    return true;
                });
            });
    }

    @Override
    @NonNull
    public <T> Mono<Void> send(@NonNull String clientId, @NonNull T object) {
        return getClientByIdAndCheck(clientId).flatMap(client -> send(client, object));
    }

    @Override
    @NonNull
    public Mono<Void> sendPower(@NonNull String clientId, double power) {
        return getClientByIdAndCheck(clientId, TRAIN_CODE)
                .flatMap(client -> send(client, new PowerEntity((byte) 4, (int) (1_000 * power))))
                .onErrorResume(NotFoundException.class, th -> {
                    log.warn("sendSwitch()", th);
                    return Mono.empty();
                });
    }

    @Override
    @NonNull
    public Mono<Void> sendSpeed(@NonNull String clientId, double speed) {
        return getClientByIdAndCheck(clientId, TRAIN_CODE)
                .flatMap(client -> send(client, new SpeedEntity((byte) 5, (int) speed)))
                .onErrorResume(NotFoundException.class, th -> {
                    log.warn("sendSpeed()", th);
                    return Mono.empty();
                });
    }

    @Override
    @NonNull
    public Mono<Void> sendSwitch(@NonNull String clientId, boolean value) {
        return getClientByIdAndCheck(clientId, SWITCH_CODE, SWITCH_WITH_SEMAPHORE_CODE)
                .flatMap(client -> send(client, new SwitchEntity((byte) 6, (byte) (value ? 1 : 0))))
                .onErrorResume(NotFoundException.class, th -> {
                    log.warn("sendSwitch()", th);
                    return Mono.empty();
                });
    }

    @Override
    @NonNull
    public Mono<Void> sendSemaphore(@NonNull String clientId, int color) {
        return getClientByIdAndCheck(clientId, SEMAPHORE_CODE, SWITCH_WITH_SEMAPHORE_CODE)
                .flatMap(client -> {
                    if (client.getClientTypeCode() == SEMAPHORE_CODE)
                        return send(client, new SemaphoreEntity((byte) 7, (short) 0, color));

                    final int i = clientId.lastIndexOf(SUBITEMS_DELIMETER);
                    if (i >= 0) {
                        try {
                            final int index = Integer.parseInt(clientId.substring(i + 1));

                            if (index >= 0 && index < client.getSubItems())
                                return send(client, new SemaphoreEntity((byte) 7, (short) index, color));
                        } catch (NumberFormatException nfe) {
                            log.warn("Parse index failed: " + clientId);
                        }
                    }

                    return Mono.error(new IllegalArgumentException(clientId + " is not contain subitems ("
                                        + client.getSubItems() + ")."));
                })
                .onErrorResume(NotFoundException.class, th -> {
                    log.warn("sendSemaphore()", th);
                    return Mono.empty();
                });
    }

    @Override
    @Nullable
    public String getClientIdByItemId(@NonNull String itemId) {
        final ClientInfo client = clientById.get(itemId);

        return client == null ? null : client.getId();
    }

    @NonNull
    private Mono<ClientInfo> getClientById(@NonNull String clientId) {
        return Mono.defer(() -> Mono.justOrEmpty(clientById.get(clientId)));
    }

    @NonNull
    private Mono<ClientInfo> getClientByIdAndCheck(@NonNull String clientId) {
        return Mono.defer(() -> {
            final ClientInfo result = clientById.get(clientId);

            return result == null ? Mono.error(new NotFoundException("Client with id=" + clientId + " not found")) : Mono.just(result);
        });
    }

    @NonNull
    private Mono<ClientInfo> getClientByIdAndCheck(@NonNull String clientId, short ... typeCodes) {
        return getClientByIdAndCheck(clientId)
                .flatMap(client -> {
                    for (short typeCode : typeCodes)
                        if (typeCode == client.getClientTypeCode())
                            return Mono.just(client);

                    return Mono.error(new IllegalArgumentException(clientId + " is not in types: " + Arrays.toString(typeCodes)));
                });
    }
}
