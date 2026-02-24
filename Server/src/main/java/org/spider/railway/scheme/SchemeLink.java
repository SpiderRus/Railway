package org.spider.railway.scheme;

import lombok.Getter;
import lombok.RequiredArgsConstructor;
import reactor.util.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;

@RequiredArgsConstructor
@Getter
public class SchemeLink {
    private final SchemeConnector from;
    private final SchemeConnector to;
    private final double length;
    private final List<SchemeCoords> intermediatePoints = new ArrayList<>();

    public SchemeLink(@NonNull SchemeConnector from, @NonNull SchemeConnector to) {
        this(from, to, 0.0);
    }

    public void addIntermediatePoint(@NonNull SchemeCoords point) {
        intermediatePoints.add(point);
    }
}
