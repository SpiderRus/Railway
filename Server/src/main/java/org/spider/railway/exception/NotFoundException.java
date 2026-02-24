package org.spider.railway.exception;

import reactor.util.annotation.NonNull;

public class NotFoundException extends Exception {
    public NotFoundException(@NonNull String message, @NonNull Throwable cause) {
        super(message, cause);
    }

    public NotFoundException(@NonNull String message) {
        super(message);
    }

    public NotFoundException(@NonNull String format, Object ... args) {
        this(String.format(format, args));
    }
}
