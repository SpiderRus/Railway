package org.spider.railway.scheme;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import reactor.util.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;

@AllArgsConstructor
@Getter
@Setter
public abstract class SchemeNode {
    private final String id;
    private final List<SchemeLink> internalLinks;
    private final List<SchemeLink> externalLinks;
    private final List<SchemeConnector> connectors;

    public SchemeNode(@NonNull String id) {
        this(id, new ArrayList<>(), new ArrayList<>(), new ArrayList<>());
    }

    protected void addConnectors(@NonNull SchemeConnector ... connectors) {
        for (SchemeConnector connector : connectors)
            this.connectors.add(connector);
    }

    public void addExternalLinks(@NonNull SchemeLink ... links) {
        for (SchemeLink link : links) {
            if (link.getFrom().getNode() == this && link.getTo().getNode() == this)
                throw new IllegalArgumentException("Internal link added as external.");

            if (link.getFrom().getNode() != this && link.getTo().getNode() != this)
                throw new IllegalArgumentException("External link must be a same node.");

            this.externalLinks.add(link);
        }
    }

    public void addBidirectionalExternalLink(@NonNull SchemeConnector connector1, @NonNull SchemeConnector connector2) {
        addExternalLinks(new SchemeLink(connector1, connector2), new SchemeLink(connector2, connector1));
    }

    protected void addInternalLinks(@NonNull SchemeLink ... links) {
        for (SchemeLink link : links)
            this.internalLinks.add(link);
    }

    public abstract boolean inBounds(@NonNull SchemeCoords point);

    public abstract double getDistance(@NonNull SchemeCoords point);

    @Override
    public String toString() {
        return "Node={id=" + this.id + "}";
    }
}
