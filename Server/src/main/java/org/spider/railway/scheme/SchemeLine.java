package org.spider.railway.scheme;

import org.spider.railway.utils.Utils;
import reactor.util.annotation.NonNull;

public class SchemeLine extends SchemeNode {
    protected final double LINE_WIDTH = 30.0;

    protected static double getLineLength(@NonNull SchemeCoords point1, @NonNull SchemeCoords point2) {
        final double dx = Math.abs(point2.getX() - point1.getX());
        final double dy = Math.abs(point2.getY() - point1.getY());

        return Math.sqrt(dx * dx + dy * dy);
    }

    public SchemeLine(@NonNull String id, @NonNull SchemeCoords connector1Coords, @NonNull SchemeCoords connector2Coords,
                      double requiredLength) {
        super(id);

        final SchemeConnector connector1 = new SchemeConnector(this, connector1Coords);
        final SchemeConnector connector2 = new SchemeConnector(this, connector2Coords);

        addConnectors(connector1, connector2);

        if (Math.abs(requiredLength - getLineLength(connector1Coords, connector2Coords)) >= 0.001)
            throw new IllegalArgumentException("Line length must be a 231mm, but is " + getLineLength(connector1Coords, connector2Coords));

        addInternalLinks(new SchemeLink(connector1, connector2, requiredLength), new SchemeLink(connector2, connector1, requiredLength));
    }

    @NonNull
    public SchemeConnector getConnector1() {
        return getConnectors().get(0);
    }

    @NonNull
    public SchemeConnector getConnector2() {
        return getConnectors().get(1);
    }

    @Override
    public boolean inBounds(@NonNull SchemeCoords point) {
        return getDistance(point) <= LINE_WIDTH / 2;
    }

    @Override
    public double getDistance(@NonNull SchemeCoords point) {
        final SchemeCoords a = getConnector1().getCoords(), b = getConnector2().getCoords();

        final double bax = b.getX() - a.getX();
        final double bay = b.getY() - a.getY();
        double t = ( (point.getX() - a.getX()) * bax + (point.getY() - a.getY()) * bay )
                        / ( Utils.square(bax) + Utils.square(bay) );

        t = Math.max(0.0, Math.min(t, 1.0));

        return Math.sqrt(Utils.square(a.getX() - point.getX() + t * bax) +
                            Utils.square(a.getY() - point.getY() + t * bay ));
    }
}
