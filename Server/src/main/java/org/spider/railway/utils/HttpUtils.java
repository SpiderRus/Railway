package org.spider.railway.utils;

import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import reactor.core.publisher.Mono;

@Slf4j
public class HttpUtils {
    private static final ResponseEntity<?> NOT_FOUND = ResponseEntity.status(HttpStatus.NOT_FOUND).build();
    private static final Mono<?> NOT_FOUND_MONO = Mono.just(NOT_FOUND);

    @SuppressWarnings("unchecked")
    public static <T> ResponseEntity<T> notFoundEntity() {
        return (ResponseEntity<T>)NOT_FOUND;
    }

    @SuppressWarnings("unchecked")
    public static <T> Mono<ResponseEntity<T>> notFoundEntityMono() {
        return (Mono<ResponseEntity<T>>)NOT_FOUND_MONO;
    }
}
