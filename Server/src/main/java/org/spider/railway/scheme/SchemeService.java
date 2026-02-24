package org.spider.railway.scheme;

import jakarta.annotation.PostConstruct;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import reactor.util.annotation.NonNull;

import java.util.List;

@Service
@Slf4j
public class SchemeService {
    // LINE
    @NonNull
    private static List<SchemeNode> testScheme() {
        SchemeCoords last = new SchemeCoords(231.0, 0.0), next;
        final SchemeG231 G1 = new SchemeG231("G1", new SchemeCoords(0,0), last);

        next = new SchemeCoords(last.getX() + 231.0, 0.0);
        final SchemeG231 G2 = new SchemeG231("G2", last, next);
        G1.addBidirectionalExternalLink(G1.getConnector2(), G2.getConnector1());
        last = next;

        next = new SchemeCoords(last.getX() + 231.0, 0.0);
        final SchemeG231 G3 = new SchemeG231("G3", last, next);
        G3.addBidirectionalExternalLink(G2.getConnector2(), G3.getConnector1());
        last = next;

        next = new SchemeCoords(last.getX() + 231.0, 0.0);
        final SchemeG231 G4 = new SchemeG231("G4", last, next);
        G4.addBidirectionalExternalLink(G3.getConnector2(), G4.getConnector1());
        last = next;

        next = new SchemeCoords(last.getX() + 231.0, 0.0);
        final SchemeG231 G5 = new SchemeG231("G5", last, next);
        G5.addBidirectionalExternalLink(G4.getConnector2(), G5.getConnector1());
        last = next;

        return List.of(G1, G2, G3, G4, G5);
    }

    private final List<SchemeNode> nodes;

    public SchemeService() {
        this.nodes = testScheme();
    }

    @PostConstruct
    private void init() {
        final SchemeCoords point = new SchemeCoords(600, 10);

        for (SchemeNode node : nodes)
            log.info("Distance for {} is {}.", node, node.getDistance(point));
    }

    @NonNull
    public SchemeCoords getStartPosition() {
        return nodes.get(0).getConnectors().get(0).getCoords();
    }
}
