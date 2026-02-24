package org.spider.railway.utils.udp;

import io.netty.channel.socket.DatagramPacket;
import lombok.extern.slf4j.Slf4j;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxSink;
import reactor.core.publisher.Mono;
import reactor.netty.Connection;
import reactor.netty.udp.UdpInbound;
import reactor.netty.udp.UdpOutbound;
import reactor.util.annotation.NonNull;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.List;

@Slf4j
public class UdpServer {
    private final Mono<? extends List<? extends Connection>> connections;

    private volatile FluxSink<DatagramPacket> sink;

    public UdpServer(int port) {
        this.connections =  listInetAddresses()
                .flatMap(host -> Mono.defer(() -> {
                    log.info("Listen {}", host);

                    reactor.netty.udp.UdpServer server = reactor.netty.udp.UdpServer
                            .create()
                            .host(host)
                            .port(port)
                            .handle(this::handle);
                    return server.warmup().then(server.bind());
                }))
                .collectList()
                .share();
    }

    private static Flux<String> listInetAddresses() {
        return Flux.create(sink -> {
                try {
                    @SuppressWarnings("BlockingMethodInNonBlockingContext")
                    final Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();

                    while (interfaces.hasMoreElements()) {
                        final Enumeration<InetAddress> inetAddresses = interfaces.nextElement().getInetAddresses();

                        while (inetAddresses.hasMoreElements()) {
                            final InetAddress address = inetAddresses.nextElement();

                            if (address instanceof Inet4Address)
                                sink.next(address.getHostAddress());
                        }
                    }

                    sink.complete();
                } catch (SocketException e) {
                    sink.error(e);
                }
            });
    }

    private Mono<Void> handle(UdpInbound in, UdpOutbound out) {
        return Mono.defer(() -> in.receiveObject()
                .concatMap(obj -> {
                    if (obj instanceof DatagramPacket packet) {
                        final FluxSink<DatagramPacket> sink = this.sink;

                        if (sink != null)
                            sink.next(packet.retain());
                    } else
                        log.warn("Unknown packet: {}", obj.getClass().getName());

                    return Mono.empty();
                }).then());
    }

    public void start() {
    }

    public void destroy() {
    }

    @NonNull
    public Flux<DatagramPacket> subscribe() {
        return Flux.usingWhen(this.connections, __ -> Flux.create(sink -> {
                if (this.sink == null)
                    this.sink = sink;
                else
                    sink.error(new IllegalStateException("Too many subscribers"));
            }), connections -> {
                    this.sink = null;

                    return Mono.when(Flux.fromIterable(connections)
                            .flatMap(connection -> {
                                connection.dispose();
                                return connection.onDispose();
                            }));
                });
    }
}
