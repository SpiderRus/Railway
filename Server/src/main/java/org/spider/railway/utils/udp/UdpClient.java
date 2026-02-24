package org.spider.railway.utils.udp;

import io.netty.buffer.ByteBuf;
import lombok.extern.slf4j.Slf4j;
import lombok.val;
import org.apache.commons.lang3.stream.Streams;
import org.spider.railway.utils.ReactorUtils;
import org.spider.railway.utils.batch.OrderedProcessor;
import reactor.core.publisher.Flux;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoSink;
import reactor.core.publisher.Sinks;
import reactor.netty.NettyOutbound;
import reactor.netty.udp.UdpInbound;
import reactor.netty.udp.UdpOutbound;
import reactor.util.annotation.NonNull;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuples;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Collection;
import java.util.function.Function;
import java.util.stream.Collectors;

@Slf4j
public class UdpClient {
    private static final int DEFAULT_BUFFER_SIZE = 1436;

    private final Mono<Void> src;

    private final Sinks.Many<Tuple2<MonoSink<Void>, Collection<Function<ByteBuf, Boolean>>>> sink =
            Sinks.many().multicast().onBackpressureBuffer(1_000);

    private final OrderedProcessor<Function<ByteBuf, Boolean>, MonoSink<Void>> processor;

    public UdpClient(@NonNull final InetSocketAddress address) {
        this.src = Mono.defer(() -> {
            reactor.netty.udp.UdpClient client = reactor.netty.udp.UdpClient
                    .create()
                    .remoteAddress(() -> address)
                    .handle(this::handle);

            return client.warmup().then(client.connect()).then();
        }).cache();

        this.processor = new OrderedProcessor<>(tuples ->
            src.then(Mono.<Void>create(sink ->
                    this.sink.tryEmitNext(Tuples.of(sink, Streams.of(tuples).map(Tuple2::getT1).collect(Collectors.toList())))
                ))
                .doOnSuccess(__ -> ReactorUtils.schedule(tuples, t -> () -> t.getT2().success()))
                .doOnError(th -> ReactorUtils.error(tuples, th))
        );
    }

    private Mono<Void> handle(UdpInbound in, UdpOutbound out) {
        return sink.asFlux()
                .concatMap(item -> out.send(writeAll(out, item.getT2())).then()
                    .doOnSuccess(__ -> item.getT1().success())
                    .onErrorResume(th -> {
                        item.getT1().error(th);
                        return Mono.empty();
                    })
                ).then();
    }

    private static Flux<ByteBuf> writeAll(NettyOutbound out, Collection<Function<ByteBuf, Boolean>> writers) {
        return Flux.create(sink -> {
            ByteBuf buf = out.alloc().buffer(DEFAULT_BUFFER_SIZE);

            for (val writer : writers)
                if (!write(buf, writer)) {
                    sink.next(buf);
                    buf = out.alloc().buffer(DEFAULT_BUFFER_SIZE);
                    write(buf, writer);
                }

            sink.next(buf);
            sink.complete();
        });
    }

    private static boolean write(ByteBuf buf, Function<ByteBuf, Boolean> writer) {
        final int index = buf.writerIndex();
        buf.writerIndex(index + 2);

        if (writer.apply(buf)) {
            buf.setShortLE(index, buf.writerIndex() - index - 2);
            return true;
        }

        buf.writerIndex(index);
        return false;
    }

    public Mono<Void> send(Function<ByteBuf, Boolean> writer) {
        return Mono.create(sink -> processor.process(writer, sink));
    }
}
