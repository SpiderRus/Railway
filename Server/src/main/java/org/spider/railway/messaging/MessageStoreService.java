package org.spider.railway.messaging;

import io.r2dbc.spi.Row;
import io.r2dbc.spi.Statement;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.stream.Streams;
import org.spider.railway.item.SchemeItem;
import org.spider.railway.messaging.message.SemaphoreStatusMessage;
import org.spider.railway.messaging.message.StatusMessage;
import org.spider.railway.messaging.message.SwitchStatusMessage;
import org.spider.railway.messaging.message.TrainStatusMessage;
import org.spider.railway.utils.ReactorUtils;
import org.spider.railway.utils.Utils;
import org.spider.railway.utils.batch.OrderedProcessor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.r2dbc.core.DatabaseClient;
import org.springframework.stereotype.Service;
import reactor.core.Disposable;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxSink;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuple3;
import reactor.util.function.Tuples;

import java.time.OffsetDateTime;
import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

@Slf4j
@Service
public class MessageStoreService {
    @AllArgsConstructor
    @Getter
    private static class StatusEntity {
        private SchemeItem.Type type;
        private final String itemId;
        private final OffsetDateTime timestamp;
        private final OffsetDateTime receiveTimestamp;
        private final OffsetDateTime correctedTimestamp;
        private final Integer speed;
        private final Double power;
        private final OffsetDateTime schemeMarkerTime;
        private final double internalTemp;
        private final long version;
        private final Integer semaphoreColor;
        private final Boolean switchOn;
    }

    private static abstract class StatusMessageImpl implements StatusMessage {
        protected final StatusEntity entity;

        StatusMessageImpl(@NonNull StatusEntity entity) {
            this.entity = entity;
        }

        @Override
        @NonNull
        public String getId() {
            return entity.getItemId();
        }

        @Override
        @NonNull
        public OffsetDateTime getReceiveTimestamp() {
            return entity.getReceiveTimestamp();
        }

        @Override
        @NonNull
        public OffsetDateTime getTimestamp() {
            return entity.getTimestamp();
        }

        @Override
        @NonNull
        public SchemeItem.Type getItemType() {
            return entity.getType();
        }

        @Override
        public double getInternalTemp() {
            return entity.getInternalTemp();
        }

        @Override
        public long getVersion() {
            return entity.getVersion();
        }

        @Override
        @NonNull
        public OffsetDateTime getCorrectedTimestamp() {
            return entity.getCorrectedTimestamp();
        }
    }

    private static class TrainStatusMessageImpl extends StatusMessageImpl implements TrainStatusMessage {
        TrainStatusMessageImpl(@NonNull StatusEntity entity) {
            super(entity);
        }

        @Override
        @NonNull
        public OffsetDateTime getSchemeMarkerTime() {
            return entity.getSchemeMarkerTime();
        }

        @Override
        public int getSpeed() {
            return entity.getSpeed();
        }

        @Override
        public double getPower() {
            return entity.getPower();
        }
    }

    private static class SwitchStatusMessageImpl extends StatusMessageImpl implements SwitchStatusMessage {
        SwitchStatusMessageImpl(@NonNull StatusEntity entity) {
            super(entity);
        }

        @Override
        public boolean getState() {
            return entity.getSwitchOn();
        }
    }

    private static class SemaphoreStatusMessageImpl extends StatusMessageImpl implements SemaphoreStatusMessage {
        SemaphoreStatusMessageImpl(@NonNull StatusEntity entity) {
            super(entity);
        }

        @Override
        public int getColor() {
            return entity.getSemaphoreColor();
        }
    }

    private static final String INSERT_SQL = "INSERT INTO status_log " +
            "(item_id, timestamp, receive_timestamp, corrected_timestamp, item_type, internal_temp, version, " +
                    "marker_timestamp, speed, power, semaphore_color, switch_state) " +
            "VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)";

    private static final String BATCH_LOAD_SQL = "SELECT " +
            "item_id, timestamp, receive_timestamp, corrected_timestamp, item_type, speed, power, marker_timestamp, " +
            "internal_temp, version, semaphore_color, switch_state " +
            "FROM status_log " +
            "WHERE corrected_timestamp >= $1 AND corrected_timestamp <= $2 AND item_id in (select * from unnest(array[$3]::varchar[])) " +
            "ORDER BY corrected_timestamp";

    private static final String LOAD_MESSAGE_SQL = "SELECT " +
            "item_id, timestamp, receive_timestamp, corrected_timestamp, item_type, speed, power, marker_timestamp, " +
            "internal_temp, version, semaphore_color, switch_state " +
            "FROM status_log " +
            "WHERE item_id = $1 AND corrected_timestamp = $2";

    private static final String ALL_ITEMS_LOAD_SQL = "SELECT DISTINCT item_id FROM status_log " +
                                                     "ORDER BY item_id";

    private static final String ITEMS_IDS_LOAD_SQL = "SELECT item_id, item_type, max(corrected_timestamp) as stamp FROM status_log " +
            "GROUP BY item_id, item_type";

    private static final String LAST_MESSAGES_SQL = "SELECT item_id, max(corrected_timestamp) FROM status_log " +
            "GROUP BY item_id " +
            "ORDER BY item_id";

    private static final String BATCH_INSERT_SQL = """
        INSERT INTO status_log
            "(item_id, timestamp, receive_timestamp, corrected_timestamp, item_type, internal_temp, version,
                    "marker_timestamp, speed, power, semaphore_color, switch_state)
            select unnest(array[$1]::varchar[]),
                unnest(array[$2]::timestamptz[]),
                unnest(array[$3]::timestamptz[]),
                unnest(array[$4]::timestamptz[]),
                unnest(array[$5]::varchar[]),
                unnest(array[$6]::decimal[]),
                unnest(array[$7]::smallint[]),
                unnest(array[$8]::timestamptz[]),
                unnest(array[$9]::integer[]),
                unnest(array[$10]::decimal[]),
                unnest(array[$11]::integer[]),
                unnest(array[$12]::boolean[])
            """;

    private static void fillDataArray(@NonNull StatusMessage message, @NonNull Object[] array) {
        array[0] = message.getId();
        array[1] = message.getTimestamp();
        array[2] = message.getReceiveTimestamp();
        array[3] = message.getCorrectedTimestamp();
        array[4] = message.getItemType().name();
        array[5] = message.getInternalTemp();
        array[6] = message.getVersion();

        if (message instanceof TrainStatusMessage trainStatusMessage) {
            array[7] = trainStatusMessage.getSchemeMarkerTime();
            array[8] = trainStatusMessage.getSpeed();
            array[9] = trainStatusMessage.getPower();
        } else if (message instanceof SemaphoreStatusMessage semaphoreStatusMessage) {
            array[10] = semaphoreStatusMessage.getColor();
        } else if (message instanceof SwitchStatusMessage switchStatusMessage) {
            array[11] = switchStatusMessage.getState();
        }
    }

    private static void fillDataArray(@NonNull StatusMessage message, int index, @NonNull Object[][] array) {
        array[0][index] = message.getId();
        array[1][index] = Utils.odtFormatter.format(message.getTimestamp());
        array[2][index] = Utils.odtFormatter.format(message.getReceiveTimestamp());
        array[3][index] = Utils.odtFormatter.format(message.getCorrectedTimestamp());
        array[4][index] = message.getItemType().name();
        array[5][index] = Double.toString(message.getInternalTemp());
        array[6][index] = Utils.toString(message.getVersion());

        if (message instanceof TrainStatusMessage trainStatusMessage) {
            array[7][index] = trainStatusMessage.getSchemeMarkerTime() == null
                                    ? null : Utils.odtFormatter.format(trainStatusMessage.getSchemeMarkerTime());
            array[8][index] = Utils.toString(trainStatusMessage.getSpeed());
            array[9][index] = Double.toString(trainStatusMessage.getPower());
        } else if (message instanceof SemaphoreStatusMessage semaphoreStatusMessage) {
            array[10][index] = Utils.toString(semaphoreStatusMessage.getColor());
        } else if (message instanceof SwitchStatusMessage switchStatusMessage) {
            array[11][index] = Boolean.toString(switchStatusMessage.getState());
        }
    }

    @NonNull
    private static Statement setData(@NonNull Statement statement, @NonNull Object[] data) {
       statement =  statement.bind(0, data[0])
                .bind(1, data[1])
                .bind(2, data[2])
                .bind(3, data[3])
                .bind(4, data[4])
                .bind(5, data[5])
                .bind(6, data[6]);

       statement = data[7] == null ? statement.bindNull(7, OffsetDateTime.class) : statement.bind(7, data[7]);
       statement = data[8] == null ? statement.bindNull(8, Integer.class) : statement.bind(8, data[8]);
       statement = data[9] == null ? statement.bindNull(9, Double.class) : statement.bind(9, data[9]);
       statement = data[10] == null ? statement.bindNull(10, Integer.class) : statement.bind(10, data[10]);
       statement = data[11] == null ? statement.bindNull(11, Boolean.class) : statement.bind(11, data[11]);

       return statement;
    }

    @NonNull
    private static StatusEntity convert(@NonNull Row row) {
        //noinspection DataFlowIssue
        return new StatusEntity(
                SchemeItem.Type.valueOf(row.get(4, String.class)),
                row.get(0, String.class),
                row.get(1, OffsetDateTime.class),
                row.get(2, OffsetDateTime.class),
                row.get(3, OffsetDateTime.class),
                row.get(5, Integer.class),
                row.get(6, Double.class),
                row.get(7, OffsetDateTime.class),
                row.get(8, Double.class),
                row.get(9, Long.class),
                row.get(10, Integer.class),
                row.get(11, Boolean.class)
        );
    }

    @Nullable
    private static StatusMessage toMessage(@NonNull StatusEntity entity) {
        return switch (entity.getType()) {
            case TRAIN -> new TrainStatusMessageImpl(entity);
            case SWITCH -> new SwitchStatusMessageImpl(entity);
            case SEMAPHORE -> new SemaphoreStatusMessageImpl(entity);
            default -> null;
        };
    }

    private final MessagingService messagingService;

    private final DatabaseClient databaseClient;

    private final OrderedProcessor<StatusMessage, Void> processor;

    private final OrderedProcessor<Tuple3<String, OffsetDateTime, OffsetDateTime>, FluxSink<StatusEntity>> loadProcessor;

    private Disposable disposable;

    @Autowired
    public MessageStoreService(MessagingService messagingService, DatabaseClient databaseClient) {
        this.messagingService = messagingService;
        this.databaseClient = databaseClient;

        this.processor = new OrderedProcessor<>(tuples -> {
            final List<StatusMessage> messages = Streams.of(tuples).map(Tuple2::getT1).toList();

            return (messages.size() > 1 ? saveAll(messages) : save(messages.get(0)))
                    .then()
                    .onErrorResume(th -> {
                        log.error("save()", th);

                        return Mono.empty();
                    })
                    .subscribeOn(Schedulers.boundedElastic());
        });

        this.loadProcessor = new OrderedProcessor<>(tuples ->
            ReactorUtils.handleResult(getMessages(Streams.of(tuples).map(Tuple2::getT1)), tuples, (tuple, message) ->
                    tuple.getT1().equals(message.getItemId())
                        && Utils.isAfterOrEquals(message.getCorrectedTimestamp(), tuple.getT2())
                        && Utils.isBeforeOrEquals(message.getCorrectedTimestamp(), tuple.getT3())
                ));
    }

    @PostConstruct
    private void init() {
        this.disposable = messagingService
                .subscribeStatus()
                .subscribe(processor::process);
    }

    @PreDestroy
    private void destroy() {
        this.disposable.dispose();
    }

    @NonNull
    public Flux<? extends StatusMessage> getMessages(@NonNull String id, @NonNull OffsetDateTime from, @NonNull OffsetDateTime to) {
        return ReactorUtils.publishSingle(Flux.<StatusEntity>create(sink -> loadProcessor.process(Tuples.of(id, from, to), sink)))
                .concatMap(entity -> Mono.justOrEmpty(toMessage(entity)));
    }

    @NonNull
    public Flux<String> getAllItemIds() {
        return databaseClient.sql(ALL_ITEMS_LOAD_SQL)
                .map((row, meta) -> row.get(0, String.class))
                .all();
    }

    @NonNull
    public Flux<StatusMessage> getAllItems() {
        return databaseClient.sql(LAST_MESSAGES_SQL)
                .map((row, meta) -> Tuples.of(
                        Objects.requireNonNull(row.get(0, String.class)),
                        Objects.requireNonNull(row.get(1, OffsetDateTime.class)))
                )
                .all()
                .flatMapSequential(tuple -> loadMessage(tuple.getT1(), tuple.getT2()), 10);
    }

    @NonNull
    private Mono<StatusMessage> loadMessage(@NonNull String itemId, @NonNull OffsetDateTime correctedTime) {
        return databaseClient.sql(LOAD_MESSAGE_SQL)
                .bind(0, itemId)
                .bind(1, correctedTime)
                .map((row, meta) -> toMessage(convert(row)))
                .one();
    }

    @NonNull
    public Flux<String> getItemIdsForTypes(@NonNull SchemeItem.Type ... types) {
        return Flux.defer(() -> {
            if (types.length == 0)
                return Flux.empty();

            return databaseClient.sql(ITEMS_IDS_LOAD_SQL)
                    .map((row, meta) -> Tuples.of(
                            Objects.requireNonNull(row.get(0, String.class)),
                            Objects.requireNonNull(row.get(1, String.class)),
                            Objects.requireNonNull(row.get(2, OffsetDateTime.class)))
                    )
                    .all()
                    .collectMultimap(Tuple2::getT1, tuple -> Tuples.of(tuple.getT2(), tuple.getT3()))
                    .flatMapMany(map -> {
                        final Set<String> strTypes = Stream.of(types).map(Enum::name).collect(Collectors.toSet());
                        return Flux.fromStream(map.entrySet().stream()
                                .filter(en -> {
                                    Tuple2<String, OffsetDateTime> max = null;

                                    for (var tuple : en.getValue())
                                        if (max == null || max.getT2().isBefore(tuple.getT2()))
                                            max = tuple;

                                    return strTypes.contains(Objects.requireNonNull(max).getT1());
                                })
                                .map(Map.Entry::getKey));
                    });
        });
    }

    @NonNull
    private Flux<StatusEntity> getMessages(@NonNull Collection<String> ids, @NonNull OffsetDateTime from, @NonNull OffsetDateTime to) {
        return databaseClient.sql(BATCH_LOAD_SQL)
                    .bind(0, from)
                    .bind(1, to)
                    .bind(2, ids.toArray())
                .map((row, meta) -> convert(row))
                .all();
    }

    // id, from, to
    @NonNull
    private Flux<StatusEntity> getMessages(@NonNull Stream<Tuple3<String, OffsetDateTime, OffsetDateTime>> params) {
        return Flux.defer(() -> {
            final HashSet<String> ids = new HashSet<>(10);
            OffsetDateTime min = OffsetDateTime.now(), max = min.minusYears(10L);

            for (var iterator = params.iterator(); iterator.hasNext(); ) {
                final var tuple = iterator.next();

                ids.add(tuple.getT1());

                min = Utils.min(min, tuple.getT2());
                max = Utils.max(max, tuple.getT3());
            }

            return getMessages(ids, min, max);
        });
    }
    @NonNull
    private Mono<Long> save(@NonNull StatusMessage message) {
        return Mono.defer(() -> {
            final Object[] data = new Object[12];
            fillDataArray(message, data);

            return databaseClient.inConnection(connection ->
                    Mono.from(setData(connection.createStatement(INSERT_SQL), data).execute())
                            .flatMap(result -> Mono.from(result.getRowsUpdated())));
        });
    }

    @NonNull
    private Mono<Long> saveAll(@NonNull Collection<StatusMessage> messages) {
        return Mono.defer(() -> {
            final Object[][] data = new Object[12][messages.size()];

            int index = 0;
            for (StatusMessage message : messages)
                fillDataArray(message, index++, data);

            return databaseClient.inConnection(connection -> {
                Statement statement = connection.createStatement(BATCH_INSERT_SQL);

                for (int i = 0; i < data.length; i++)
                    statement = statement.bind(i, data[i]);

                return Mono.from(statement.execute()).flatMap(result -> Mono.from(result.getRowsUpdated()));
            });
        });
    }
}
