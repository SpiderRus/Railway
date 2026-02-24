package org.spider.railway.scheme;

import lombok.AllArgsConstructor;
import lombok.Getter;
import org.spider.railway.item.SchemeItem;

@AllArgsConstructor
@Getter
public class SchemeConnector {
    private final SchemeNode node;

    private final SchemeCoords coords;
}
