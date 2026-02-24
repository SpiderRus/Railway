package org.spider.railway.item;

import lombok.Getter;
import reactor.util.annotation.NonNull;
import reactor.util.annotation.Nullable;

import java.util.HashMap;
import java.util.Map;

public interface SchemeItem {
    @Getter
    enum Type {
        TRAIN(0),
        SWITCH(1),
        SEMAPHORE(2);

        private final int code;

        Type(int code) {
            this.code = code;
        }

        private static final Map<Integer, Type> byCodeMap = new HashMap<>();

        static {
            for (Type type: Type.values())
                byCodeMap.put(type.code, type);
        }

        @Nullable
        public static Type of(int code) {
            return byCodeMap.get(code);
        }
    }

    @NonNull
    String getId();

    @NonNull
    Map<String, Object> getProperties();
}
