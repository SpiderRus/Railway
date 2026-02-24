package org.spider.railway.logger;

import io.r2dbc.spi.Statement;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.stream.Streams;
import org.spider.railway.messaging.MessagingService;
import org.spider.railway.messaging.message.LogMessage;
import org.spider.railway.utils.Utils;
import org.spider.railway.utils.batch.OrderedProcessor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.r2dbc.core.ConnectionAccessor;
import org.springframework.stereotype.Service;
import reactor.core.Disposable;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.function.Tuple2;

import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeFormatterBuilder;
import java.util.Collection;
import java.util.List;

import static java.time.format.DateTimeFormatter.ISO_LOCAL_DATE_TIME;

@Slf4j
@Service
public class LoggerServiceImpl {
    private static final String INSERT_SQL = "INSERT INTO scheme_items_log " +
                                             "(item_id, timestamp, receive_timestamp, core, level, tag, message) " +
                                             "VALUES($1, $2, $3, $4, $5, $6, $7)";

    private static final String BATCH_INSERT_SQL = "INSERT INTO scheme_items_log " +
                                                    "(item_id, timestamp, receive_timestamp, core, level, tag, message) " +
            "select unnest(array[$1]::varchar[]), " +
            "unnest(array[$2]::timestamptz[]), " +
            "unnest(array[$3]::timestamptz[]), " +
            "unnest(array[$4]::smallint[]), " +
            "unnest(array[$5]::smallint[]), " +
            "unnest(array[$6]::varchar[]), " +
            "unnest(array[$7]::varchar[])";

    private final MessagingService messagingService;

    private final ConnectionAccessor connectionAccessor;

    private final OrderedProcessor<LogMessage, Void> processor;

    private Disposable disposable;

    @Autowired
    public LoggerServiceImpl(MessagingService messagingService, ConnectionAccessor connectionAccessor) {
        this.messagingService = messagingService;
        this.connectionAccessor = connectionAccessor;

        this.processor = new OrderedProcessor<>(tuples -> {
                List<LogMessage> messages = Streams.of(tuples).map(Tuple2::getT1).toList();

                return (messages.size() > 1 ? saveAll(messages) : save(messages.get(0)))
                        .then()
                        .onErrorResume(th -> {
                            log.error("save()", th);
                            return Mono.empty();
                        })
                        .subscribeOn(Schedulers.boundedElastic());
            });
    }

    @PostConstruct
    public void init() {
        this.disposable = messagingService.subscribeLog()
                .subscribe(message -> {
                    log.info("Receive log: {}", message);

                    processor.process(message);
                });
    }

    @PreDestroy
    public void destroy() {
        this.disposable.dispose();
    }

    @NonNull
    private Mono<Long> save(@NonNull LogMessage message) {
        return connectionAccessor.inConnection(connection ->
                Mono.from(connection.createStatement(INSERT_SQL)
                    .bind(0, message.getId())
                    .bind(1, message.getTimestamp())
                    .bind(2, message.getReceiveTimestamp())
                    .bind(3, message.getCore())
                    .bind(4, message.getLevel())
                    .bind(5, message.getTag())
                    .bind(6, message.getMessage()).execute()
                 )
                .flatMap(result -> Mono.from(result.getRowsUpdated())));
    }

    @NonNull
    private Mono<Long> saveAll(@NonNull Collection<LogMessage> messages) {
        return Mono.defer(() -> {
            final Object[][] data = new Object[7][messages.size()];

            int index = 0;
            for (LogMessage message : messages) {
                data[0][index] = message.getId();
                data[1][index] = Utils.odtFormatter.format(message.getTimestamp());
                data[2][index] = Utils.odtFormatter.format(message.getReceiveTimestamp());
                data[3][index] = message.getCore();
                data[4][index] = message.getLevel();
                data[5][index] = message.getTag();
                data[6][index] = message.getMessage();
                index++;
            }

            return connectionAccessor.inConnection(connection -> {
                Statement statement = connection.createStatement(BATCH_INSERT_SQL);

                for (int i = 0; i < data.length; i++)
                    statement = statement.bind(i, data[i]);

                return Mono.from(statement.execute()).flatMap(result -> Mono.from(result.getRowsUpdated()));
            });
        });
    }
}
