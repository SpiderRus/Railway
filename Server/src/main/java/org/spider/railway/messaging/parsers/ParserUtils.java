package org.spider.railway.messaging.parsers;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import io.netty.channel.socket.DatagramPacket;
import lombok.extern.slf4j.Slf4j;
import reactor.core.publisher.Flux;
import reactor.util.function.Tuple3;
import reactor.util.function.Tuples;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

@Slf4j
public class ParserUtils {
    public static Flux<Tuple3<Byte, InetSocketAddress, ByteBuf>> parsePackets(Flux<DatagramPacket> src) {
        final AtomicInteger subscribers = new AtomicInteger();

        return src.concatMap(packet -> {
                    final ByteBuf buf = packet.content();
                    final ArrayList<Tuple3<Byte, InetSocketAddress, ByteBuf>> result = new ArrayList<>();

                    while (buf.readableBytes() > 0) {
                        if (buf.readableBytes() < 3) {
                            log.warn("Unknown header length=" + buf.readableBytes());
                            break;
                        }

                        final short packetLength = buf.readShortLE();
                        final byte code = buf.readByte();

                        result.add(Tuples.of(code, packet.sender(), buf.duplicate().writerIndex(buf.readerIndex() + packetLength - 1)
                                .retain(subscribers.get())));
                        buf.skipBytes(packetLength - 1);
                    }

                    packet.release();

                    if (result.size() > 1)
                        log.info("Receive > 1 messages from one packet - {}", result.size());

                    return Flux.fromIterable(result);
                })
                .share()
                .doOnSubscribe(__ -> {
                    subscribers.incrementAndGet();
                    log.info("Subscribe {}.", subscribers.get());
                })
                .doFinally(__ -> subscribers.decrementAndGet());
    }
}
