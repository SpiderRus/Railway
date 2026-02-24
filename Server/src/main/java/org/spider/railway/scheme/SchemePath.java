package org.spider.railway.scheme;

import lombok.AllArgsConstructor;
import lombok.Getter;
import reactor.util.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;

@AllArgsConstructor
@Getter
public class SchemePath {
    private final List<SchemeCoords> path;

    @NonNull
    public SchemePath add(@NonNull SchemeCoords coords) {
        final ArrayList<SchemeCoords> copy = new ArrayList<>(path);
        copy.add(coords);
        return new SchemePath(copy);
    }
}
