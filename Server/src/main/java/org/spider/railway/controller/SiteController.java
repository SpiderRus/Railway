package org.spider.railway.controller;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import lombok.extern.slf4j.Slf4j;
import org.springframework.core.io.ClassPathResource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Mono;
import reactor.util.annotation.NonNull;

import java.io.IOException;
import java.time.Duration;
import java.util.Locale;
import java.util.concurrent.ExecutionException;

@Slf4j
//@RestController
//@RequestMapping("site")
public class SiteController {
    private static final Duration CACHE_DURATION = Duration.ofHours(1L);

    private static final ResponseEntity<byte[]> NOT_FOUND = ResponseEntity.status(HttpStatus.TEMPORARY_REDIRECT)
            .header(HttpHeaders.LOCATION, "/site/NotFound.html").build();

    private static final Mono<ResponseEntity<byte[]>> NOT_FOUND_MONO = Mono.just(NOT_FOUND);


    private final LoadingCache<String, Mono<ResponseEntity<byte[]>>> cache = CacheBuilder.newBuilder()
            .expireAfterAccess(CACHE_DURATION)
            .maximumSize(100)
            .build(new CacheLoader<>() {
                @Override
                @NonNull
                public Mono<ResponseEntity<byte[]>> load(@NonNull String path) {
                    return Mono.<ResponseEntity<byte[]>>create(sink -> {
                        final ClassPathResource resource = new ClassPathResource("/site/" + path);

                        if (resource.exists()) {
                            try {
                                sink.success(new ResponseEntity<>(resource.getContentAsByteArray(),
                                        headers(new HttpHeaders(), path), HttpStatus.OK));
                            } catch (IOException e) {
                                log.error("readContent()", e);

                                sink.success(ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build());
                            }
                        } else {
                            log.warn("Path {} not found.", path);

                            sink.success(NOT_FOUND);
                        }
                    }).cache(SiteController::okDuration, SiteController::errorDuration, SiteController::emptyDuration);
                }
            });

    @GetMapping(value = "**")
    public Mono<ResponseEntity<byte[]>> getContent(ServerHttpRequest request) throws ExecutionException {
        String path = request.getPath().value();

        if (!path.startsWith("/site"))
            return NOT_FOUND_MONO;

        path = path.endsWith("/") ? path.substring("/site/".length()) : path.substring("/site".length());

        if (path.isEmpty())
            path = "index.html";

        return cache.get(path);
    }

    @NonNull
    private static HttpHeaders headers(@NonNull HttpHeaders headers, String fileName) {
        String contentType = MediaType.APPLICATION_OCTET_STREAM_VALUE;

        if (fileName != null) {
            final int i = fileName.lastIndexOf('.');

            if (i >= 0)
                contentType = switch (fileName.substring(i + 1).toLowerCase(Locale.ROOT)) {
                    case "html", "htm" -> MediaType.TEXT_HTML_VALUE;
                    case "css" -> "text/css";
                    case "js" -> "text/javascript";
                    case "gif" -> MediaType.IMAGE_GIF_VALUE;
                    case "jpg", "jpeg" -> MediaType.IMAGE_JPEG_VALUE;
                    case "png" -> MediaType.IMAGE_PNG_VALUE;
                    case "svg" -> "image/svg+xml";
                    default -> contentType;
                };
        }

        headers.set("Content-Type", contentType);

        // headers.set("Cache-Control", "public, max-age=" + CACHE_DURATION.get(ChronoUnit.SECONDS));

        return headers;
    }

    @NonNull
    private static Duration okDuration(@NonNull ResponseEntity<byte[]> en) {
        return en.getStatusCode() == HttpStatus.OK ? CACHE_DURATION : Duration.ZERO;
    }

    @NonNull
    private static Duration errorDuration(@NonNull Throwable t) {
        return Duration.ZERO;
    }

    @NonNull
    private static Duration emptyDuration() {
        return Duration.ZERO;
    }
}
