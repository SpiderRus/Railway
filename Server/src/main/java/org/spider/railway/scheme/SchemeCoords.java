package org.spider.railway.scheme;

import lombok.Getter;
import lombok.RequiredArgsConstructor;
import lombok.ToString;

import java.util.Objects;

@RequiredArgsConstructor
@Getter
@ToString
public class SchemeCoords {
    private static final double EQ_MULTIPLIER = 1_000.0;

    private final double x;
    private final double y;

    @Override
    public int hashCode() {
        return Objects.hash((long) (x * EQ_MULTIPLIER), (long) (y * EQ_MULTIPLIER));
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;

        if (obj instanceof SchemeCoords other)
            return (long) (x * EQ_MULTIPLIER) == (long) (other.getX() * EQ_MULTIPLIER)
                    && (long) (y * EQ_MULTIPLIER) == (long) (other.getY() * EQ_MULTIPLIER);

        return false;
    }
}
