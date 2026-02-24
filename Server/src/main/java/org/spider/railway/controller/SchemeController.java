package org.spider.railway.controller;

import jakarta.annotation.PostConstruct;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import lombok.extern.slf4j.Slf4j;
import org.spider.railway.utils.MathUtils;
import org.springframework.context.annotation.Profile;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxSink;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Schedulers;
import reactor.util.annotation.NonNull;
import reactor.util.function.Tuple2;
import reactor.util.function.Tuples;

import java.math.BigDecimal;
import java.util.*;
import java.util.stream.Collectors;

import static org.spider.railway.utils.MathUtils.*;

@RestController
@RequestMapping("/api/scheme")
@Slf4j
public class SchemeController {
    public interface IContext {
        boolean visited(@NonNull SchemeItem item);
    }

    public abstract static class ContextBase implements IContext {
        protected final Set<String> visited;

        protected ContextBase(@NonNull Set<String> visited) {
            this.visited = visited;
        }

        public ContextBase() {
            this(new HashSet<>());
        }

        @Override
        public boolean visited(@NonNull SchemeItem item) {
            if (visited.contains(item.getId()))
                return true;

            visited.add(item.getId());

            return false;
        }
    }

    public interface SchemeItemVisitor<Context extends IContext> {
        default void visit(@NonNull SchemeItem item, @NonNull Context context) {}

        default void visit(@NonNull AbstractItem item, @NonNull Context context) {
            visit((SchemeItem) item, context);
        }

        default void visit(@NonNull TwoPointItem item, @NonNull Context context) {
            visit((AbstractItem) item, context);
        }

        default void visit(@NonNull LineItem item, @NonNull Context context) {
            visit((TwoPointItem) item, context);
        }

        default void visit(@NonNull G119 item, @NonNull Context context) {
            visit((LineItem) item, context);
        }

        default void visit(@NonNull G231 item, @NonNull Context context) {
            visit((LineItem) item, context);
        }

        default void visit(@NonNull G239 item, @NonNull Context context) {
            visit((LineItem) item, context);
        }

        default void visit(@NonNull RadiusItem item, @NonNull Context context) {
            visit((TwoPointItem) item, context);
        }

        default void visit(@NonNull R1R item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull R1L item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull R2R item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull R2L item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull R9R item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull R9L item, @NonNull Context context) {
            visit((RadiusItem) item, context);
        }

        default void visit(@NonNull TreePointItem item, @NonNull Context context) {
            visit((AbstractItem) item, context);
        }

        default void visit(@NonNull SwitchLineRight item, @NonNull Context context) {
            visit((TreePointItem) item, context);
        }

        default void visit(@NonNull SwitchLineLeft item, @NonNull Context context) {
            visit((TreePointItem) item, context);
        }

        default void visit(@NonNull SwitchR2Right item, @NonNull Context context) {
            visit((TreePointItem) item, context);
        }

        default void visit(@NonNull SwitchR2Left item, @NonNull Context context) {
            visit((TreePointItem) item, context);
        }
    }

    public interface SchemeItem {
        enum Type {
            G119,
            G231,
            G239,
            R1L,
            R1R,
            R2L,
            R2R,
            R9R,
            R9L,
            SWITCH_LINE_LEFT,
            SWITCH_LINE_RIGHT,
            SWITCH_R2_LEFT,
            SWITCH_R2_RIGHT
        }

        @NonNull
        String getId();

        @NonNull
        Type getType();

        <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context);
    }

    @Getter
    @Setter
    public static abstract class AbstractItem implements SchemeItem {
        private final String id;

        private AbstractItem root;

        public AbstractItem(@NonNull String id) {
            this.id = id;
        }

        public AbstractItem(@NonNull String id, AbstractItem root) {
            this(id);
            this.root = root;
        }

        @Override
        @NonNull
        public String getId() {
            return this.id;
        }

        @Override
        public int hashCode() {
            return id.hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            return this == obj || ((obj instanceof AbstractItem) && this.id.equals(((AbstractItem)obj).id));
        }
    }

    @Getter
    @Setter
    public static abstract class TwoPointItem extends AbstractItem {
        private AbstractItem child;

        public TwoPointItem(String id, AbstractItem child) {
            super(id);

            this.child = child;
        }

        public TwoPointItem(String id, TwoPointItem child) {
            this(id, (AbstractItem) child);

            if (child != null)
                child.setRoot(this);
        }

        public TwoPointItem(String id, TreePointItem child) {
            this(id, (AbstractItem) child);

            if (child != null)
                child.setRoot(this);
        }
    }

    @Getter
    @Setter
    public static abstract class LineItem extends TwoPointItem {
        private final BigDecimal length;

        public LineItem(@NonNull String id, int length, AbstractItem child) {
            super(id, child);
            this.length = toBigDecimal(length);
        }

        public LineItem(@NonNull String id, int length, TwoPointItem child) {
            super(id, child);
            this.length = toBigDecimal(length);
        }

        public LineItem(@NonNull String id, int length, TreePointItem child) {
            super(id, child);
            this.length = toBigDecimal(length);
        }

        public LineItem(@NonNull String id, int length) {
            this(id, length, (AbstractItem) null);
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @Override
        public String toString() {
            return "Line={id=" + getId() + ", length=" + getLength() + "}";
        }
    }

    private static final BigDecimal DEGREE_180 = toBigDecimal(180);

    @NonNull
    private static BigDecimal fromDegree(int degree) {
        return toBigDecimal(degree).multiply(PI).divide(DEGREE_180, PRECISION);
    }

    @Getter
    @Setter
    public static abstract class RadiusItem extends TwoPointItem {
        private final BigDecimal radius;
        private final BigDecimal angle;

        public RadiusItem(@NonNull String id, int radius, int degree, AbstractItem child) {
            super(id, child);

            this.radius = toBigDecimal(radius);
            this.angle = fromDegree(degree);
        }

        public RadiusItem(@NonNull String id, int radius, int degree, TwoPointItem child) {
            super(id, child);

            this.radius = toBigDecimal(radius);
            this.angle = fromDegree(degree);
        }

        public RadiusItem(@NonNull String id, int radius, int degree) {
            this(id, radius, degree, null);
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @Override
        public String toString() {
            return "Radius={id=" + getId() + ", radius=" + radius + ", angle=" + angle + "}";
        }
    }

    public static class G119 extends LineItem {
        private static final int LENGTH = 119;

        public G119(String id) {
            super(id, LENGTH);
        }

        public G119(String id, AbstractItem child) {
            super(id, LENGTH, child);
        }

        public G119(String id, TwoPointItem child) {
            super(id, LENGTH, child);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.G119;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class G231 extends LineItem {
        private static final int LENGTH = 231;

        public G231(String id) {
            super(id, LENGTH);
        }

        public G231(String id, AbstractItem child) {
            super(id, LENGTH, child);
        }

        public G231(String id, TreePointItem child) {
            super(id, LENGTH, child);
        }

        public G231(String id, TwoPointItem child) {
            super(id, LENGTH, child);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.G231;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class G239 extends LineItem {
        private static final int LENGTH = 239;

        public G239(String id) {
            super(id, LENGTH);
        }

        public G239(String id, AbstractItem child) {
            super(id, LENGTH, child);
        }

        public G239(String id, TreePointItem child) {
            super(id, LENGTH, child);
        }

        public G239(String id, TwoPointItem child) {
            super(id, LENGTH, child);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.G239;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static abstract class R1 extends RadiusItem {
        private static final int RADIUS = 360;

        public R1(String id, int angle, AbstractItem child) {
            super(id, RADIUS, angle, child);
        }

        public R1(String id, int angle, TwoPointItem child) {
            super(id, RADIUS, angle, child);
        }

        public R1(String id, int angle) {
            super(id, RADIUS, angle);
        }
    }

    public static abstract class R2 extends RadiusItem {
        private static final int RADIUS = 422;

        public R2(String id, int angle, AbstractItem child) {
            super(id, RADIUS, angle, child);
        }

        public R2(String id, int angle, TwoPointItem child) {
            super(id, RADIUS, angle, child);
        }

        public R2(String id, int angle) {
            super(id, RADIUS, angle);
        }
    }

    public static abstract class R9 extends RadiusItem {
        private static final int RADIUS = 908;

        public R9(String id, int angle, AbstractItem child) {
            super(id, RADIUS, angle, child);
        }

        public R9(String id, int angle, TwoPointItem child) {
            super(id, RADIUS, angle, child);
        }

        public R9(String id, int angle) {
            super(id, RADIUS, angle);
        }
    }

    public static class R2L extends R2 {
        private static final int ANGLE = 30;

        public R2L(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R2L(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R2L(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R2L;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class R2R extends R2 {
        private static final int ANGLE = -30;

        public R2R(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R2R(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R2R(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R2R;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class R1L extends R1 {
        private static final int ANGLE = 30;

        public R1L(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R1L(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R1L(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R1L;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class R1R extends R1 {
        private static final int ANGLE = -30;

        public R1R(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R1R(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R1R(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R1R;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class R9L extends R9 {
        private static final int ANGLE = 15;

        public R9L(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R9L(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R9L(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R9L;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    public static class R9R extends R9 {
        private static final int ANGLE = -15;

        public R9R(String id, AbstractItem child) {
            super(id, ANGLE, child);
        }

        public R9R(String id, TwoPointItem child) {
            super(id, ANGLE, child);
        }

        public R9R(String id) {
            super(id, ANGLE);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.R9R;
        }

        @Override
        public <Context extends IContext> void accept(@NonNull SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }
    }

    @Getter
    @Setter
    public static abstract class TreePointItem extends AbstractItem {
        protected AbstractItem leftChild;

        protected AbstractItem rightChild;

        public TreePointItem(@NonNull String id) {
            super(id);
        }

        public TreePointItem(@NonNull String id, AbstractItem leftChild, AbstractItem rightChild) {
            super(id);

            this.leftChild = leftChild;
            this.rightChild = rightChild;
        }

        public TreePointItem(@NonNull String id, TwoPointItem leftChild, TwoPointItem rightChild) {
            this(id, (AbstractItem) leftChild, (AbstractItem) rightChild);

            if (leftChild != null)
                leftChild.setRoot(this);

            if (rightChild != null)
                rightChild.setRoot(this);
        }
    }

    public static class SwitchLineRight extends TreePointItem {
        private static final BigDecimal LINE_LENGTH = toBigDecimal(239);

        private static final BigDecimal ANGLE = fromDegree(-15);

        private static final BigDecimal RADIUS = toBigDecimal(908);

        public SwitchLineRight(@NonNull String id) {
            super(id);
        }

        public SwitchLineRight(@NonNull String id, AbstractItem leftChild, AbstractItem rightChild) {
            super(id, leftChild, rightChild);
        }

        public SwitchLineRight(@NonNull String id, TwoPointItem leftChild, TwoPointItem rightChild) {
            super(id, leftChild, rightChild);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.SWITCH_LINE_RIGHT;
        }

        @Override
        public <Context extends IContext> void accept(SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @NonNull
        public BigDecimal getLineLength() {
            return LINE_LENGTH;
        }

        @NonNull
        public BigDecimal getAngle() {
            return ANGLE;
        }

        @NonNull
        public BigDecimal getRadius() {
            return RADIUS;
        }
    }

    public static class SwitchLineLeft extends TreePointItem {
        private static final BigDecimal LINE_LENGTH = toBigDecimal(239);

        private static final BigDecimal ANGLE = fromDegree(15);

        private static final BigDecimal RADIUS = toBigDecimal(908);

        public SwitchLineLeft(@NonNull String id) {
            super(id);
        }

        public SwitchLineLeft(@NonNull String id, AbstractItem leftChild, AbstractItem rightChild) {
            super(id, leftChild, rightChild);
        }

        public SwitchLineLeft(@NonNull String id, TwoPointItem leftChild, TwoPointItem rightChild) {
            super(id, leftChild, rightChild);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.SWITCH_LINE_LEFT;
        }

        @Override
        public <Context extends IContext> void accept(SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @NonNull
        public BigDecimal getLineLength() {
            return LINE_LENGTH;
        }

        @NonNull
        public BigDecimal getAngle() {
            return ANGLE;
        }

        @NonNull
        public BigDecimal getRadius() {
            return RADIUS;
        }
    }

    public static class SwitchR2Right extends TreePointItem {
        private static final BigDecimal LINE_LENGTH = toBigDecimal(61);

        private static final BigDecimal ANGLE = fromDegree(-30);

        private static final BigDecimal RADIUS = toBigDecimal(422);

        public SwitchR2Right(@NonNull String id) {
            super(id);
        }

        public SwitchR2Right(@NonNull String id, AbstractItem leftChild, AbstractItem rightChild) {
            super(id, leftChild, rightChild);
        }

        public SwitchR2Right(@NonNull String id, TwoPointItem leftChild, TwoPointItem rightChild) {
            super(id, leftChild, rightChild);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.SWITCH_R2_RIGHT;
        }

        @Override
        public <Context extends IContext> void accept(SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @NonNull
        public BigDecimal getLineLength() {
            return LINE_LENGTH;
        }

        @NonNull
        public BigDecimal getAngle() {
            return ANGLE;
        }

        @NonNull
        public BigDecimal getRadius() {
            return RADIUS;
        }
    }

    public static class SwitchR2Left extends TreePointItem {
        private static final BigDecimal LINE_LENGTH = toBigDecimal(61);

        private static final BigDecimal ANGLE = fromDegree(30);

        private static final BigDecimal RADIUS = toBigDecimal(422);

        public SwitchR2Left(@NonNull String id) {
            super(id);
        }

        public SwitchR2Left(@NonNull String id, AbstractItem leftChild, AbstractItem rightChild) {
            super(id, leftChild, rightChild);
        }

        public SwitchR2Left(@NonNull String id, TwoPointItem leftChild, TwoPointItem rightChild) {
            super(id, leftChild, rightChild);
        }

        @Override
        @NonNull
        public Type getType() {
            return Type.SWITCH_R2_LEFT;
        }

        @Override
        public <Context extends IContext> void accept(SchemeItemVisitor<Context> visitor, Context context) {
            visitor.visit(this, context);
        }

        @NonNull
        public BigDecimal getLineLength() {
            return LINE_LENGTH;
        }

        @NonNull
        public BigDecimal getAngle() {
            return ANGLE;
        }

        @NonNull
        public BigDecimal getRadius() {
            return RADIUS;
        }
    }

    @AllArgsConstructor
    @Getter
    public static class ItemDto {
        private final String id;
        private final String type;
        private final BigDecimal x, y, angle;
    }

    @AllArgsConstructor
    @Getter
    public static class PositionedItem {
        private final SchemeItem item;
        private final BigDecimal x, y, angle;
    }

    @Getter
    @Setter
    private static class DirectionContext extends ContextBase {
        BigDecimal currentX, currentY, currentAngle;
        SchemeItem parent;

        public DirectionContext() {
            clearData();
            this.parent = null;
        }

        DirectionContext(@NonNull DirectionContext from) {
            super(from.visited);

            this.currentX = from.currentX;
            this.currentY = from.currentY;
            this.currentAngle = from.currentAngle;
            this.parent = from.parent;
        }

        private void setPositiveAngle(@NonNull BigDecimal angle) {
            BigDecimal result = angle;

            while (result.compareTo(DOUBLE_PI) >= 0)
                result = result.subtract(DOUBLE_PI);

            this.currentAngle = result;
        }

        private void setNegativeAngle(@NonNull BigDecimal angle) {
            BigDecimal result = angle;

            while (result.compareTo(MINUS_DOUBLE_PI) <= 0)
                result = result.add(DOUBLE_PI);

            this.currentAngle = result;
        }

        private void setAngle(@NonNull BigDecimal angle) {
            if (angle.compareTo(ZERO) < 0)
                setNegativeAngle(angle);
            else
                setPositiveAngle(angle);
        }

        @NonNull
        public DirectionContext rotate(@NonNull BigDecimal angle) {
            setAngle(this.currentAngle.add(angle));
            return this;
        }

        @NonNull
        public DirectionContext move(@NonNull BigDecimal dx, @NonNull BigDecimal dy) {
            final PointE p = MathUtils.rotate(dx, dy, this.currentAngle);

            this.currentX = this.currentX.add(p.x());
            this.currentY = this.currentY.add(p.y());

            return this;
        }

        @NonNull
        public DirectionContext rotate(@NonNull BigDecimal radius, @NonNull BigDecimal angle) {
            final BigDecimal rad = angle.compareTo(ZERO) < 0 ? radius.negate() : radius;
            final PointE center = MathUtils.rotate(this.currentX, this.currentY,
                                    this.currentX, this.currentY.add(rad), this.currentAngle);
            final PointE p = MathUtils.rotate(center.x(), center.y(), this.currentX, this.currentY, angle);

            this.currentX = p.x();
            this.currentY = p.y();
            setAngle(this.currentAngle.add(angle));
            return this;
        }

        public void clearData() {
            this.currentX = ZERO;
            this.currentY = ZERO;
            this.currentAngle = ZERO;
        }

        @NonNull
        public DirectionContext copy() {
            return new DirectionContext(this);
        }

        @NonNull
        public PositionedItem toPosition(@NonNull SchemeItem item) {
            return new PositionedItem(item, this.currentX, this.currentY, this.currentAngle);
        }
    }

    private final SchemeItem rootItem;

    private final Mono<List<PositionedItem>> items = listAll().collectList().cache();

    public SchemeController() {
        final R2R lastSegmentLeft1 = new R2R("1-5");
        final R2R segmentLeft1 = new R2R("1-1", new R2R("1-2", new G119("1-3", new R2R("1-4", lastSegmentLeft1))));

        final R2R lastSegmentLeft2 = new R2R("2-5");
        final R2R segmentLeft2 = new R2R("2-1", new R2R("2-2", new G119("2-3", new R2R("2-4", lastSegmentLeft2))));

        final SwitchR2Left switch3 = new SwitchR2Left("SW3");
        switch3.setRightChild(lastSegmentLeft1);
        lastSegmentLeft1.setChild(switch3);
        switch3.setLeftChild(lastSegmentLeft2);
        lastSegmentLeft2.setChild(switch3);

        final G239 lastSegmentRight1 = new G239("3-12");
        final G239 segmentRight1 = new G239("3-1", new G119("3-2", new R2R("3-3",
                                    new R2R("3-4", new R2R("3-5", new G119("3-6", new R2R("3-7",
                                    new R2R("3-8", new R2R("3-9", new G119("3-10", new G239("3-11", lastSegmentRight1
                                    )))))))))));
        final var lastSegmentRight2 = new G239("4-11");
        final var segmentRight2 = new R9L("4-1", new G119("4-2", new R1R("4-3",
                new R1R("4-4", new R1R("4-5", new G119("4-6", new R1R("4-7",
                        new R1R("4-8", new R1R("4-9", new G119("4-10", lastSegmentRight2
                        ))))))))));

        final SwitchLineRight switch5 = new SwitchLineRight("switch5", segmentRight1, segmentRight2);
        final G239 lineUp = new G239("UP-1", switch5);
        lineUp.setRoot(switch3);
        switch3.setRoot(lineUp);

        final var root1 = new R9R("root-2");
        final G231 root = new G231("root", new G231("root-1", root1));

        final SwitchLineLeft switch1 = new SwitchLineLeft("SW1");
        final SwitchR2Right switch2 = new SwitchR2Right("TestSwitch", segmentLeft1, segmentLeft2);
        switch2.setRoot(switch1);
        switch1.setRoot(switch2);

        final SwitchLineRight switch4 = new SwitchLineRight("SW4");
        switch4.setRoot(switch1);
        switch1.setLeftChild(switch4);
        root1.setChild(switch4);
        switch4.setLeftChild(root1);
        switch1.setRightChild(lastSegmentRight1);
        lastSegmentRight1.setChild(switch1);
        switch4.setRightChild(lastSegmentRight2);
        lastSegmentRight2.setChild(switch4);

        this.rootItem = root;
    }

    @PostConstruct
    protected void init() {
        findNearest(new PointE(ZERO, ZERO))
                .subscribeOn(Schedulers.boundedElastic())
                .subscribe(p -> {
                    log.info("Found: {} - {}", p.getT1().getItem().getId(), p.getT2());
                });
    }

    @NonNull
    private Mono<Tuple2<PositionedItem, BigDecimal>> findNearest(@NonNull MathUtils.PointE point) {
        return items.flatMap(items -> {
                PositionedItem result = null;
                BigDecimal minDistance = MAX_VALUE;

                for (PositionedItem item : items) {
                    BigDecimal distance = minDistance;
                    final PointE startPoint = MathUtils.rotate(item.getX(), item.getY(), item.getAngle());

                    if (item.getItem() instanceof LineItem line) {
                        final PointE endPoint = MathUtils.rotate(startPoint.x(), startPoint.y(),
                                        startPoint.x().add(line.getLength()), startPoint.y(), item.getAngle());
                        distance = MathUtils.getDistanceToLine(point, startPoint, endPoint);
                    }

                    if (distance.compareTo(minDistance) < 0) {
                        result = item;
                        minDistance = distance;
                    }
                }

                return result == null ? Mono.empty() : Mono.just(Tuples.of(result, minDistance));
            });
    }

    public record AllResult(
        @NonNull BigDecimal width,
        @NonNull BigDecimal height,

        @NonNull BigDecimal offsetX,

        @NonNull BigDecimal offsetY,
        @NonNull List<ItemDto> items
    ) {}

    @GetMapping(produces = MediaType.APPLICATION_JSON_VALUE)
    @NonNull
    public Mono<AllResult> getAll() {
        return items
                .map(items -> {
                    if (items.isEmpty())
                        return new AllResult(ZERO, ZERO, ZERO, ZERO, Collections.emptyList());

                    BigDecimal minX = new BigDecimal(Long.MAX_VALUE),
                               minY = new BigDecimal(Long.MAX_VALUE),
                               maxX = new BigDecimal(Long.MIN_VALUE),
                               maxY = new BigDecimal(Long.MIN_VALUE);

                    for (PositionedItem item : items) {
                        minX = minX.min(item.getX());
                        maxX = maxX.max(item.getX());
                        minY = minY.min(item.getY());
                        maxY = maxY.max(item.getY());
                    }

                    final BigDecimal width = maxX.subtract(minX).abs();
                    final BigDecimal height = maxY.subtract(minY).abs();
                    final BigDecimal offsetX = minX.negate();
                    final BigDecimal offsetY = minY.negate();

                    return new AllResult(width, height, offsetX, offsetY,
                            items.stream().map(p -> new ItemDto(p.getItem().getId(), p.getItem().getType().name(),
                                        round(p.getX()), round(p.getY()), round(p.getAngle()))).collect(Collectors.toList()));
                });
    }

    private static class ListVisitor implements SchemeItemVisitor<DirectionContext> {
        private final FluxSink<PositionedItem> sink;

        ListVisitor(FluxSink<PositionedItem> sink) {
            this.sink = sink;
        }

        @Override
        public void visit(@NonNull LineItem item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.move(item.getLength(), ZERO);
                } else {
                    context.move(item.getLength(), ZERO);
                    sink.next(context.copy().rotate(MINUS_PI).toPosition(item));
                }

                context.setParent(item);

                if (item.getChild() != null)
                    item.getChild().accept(this, context);

                if (!isFirst && item.getRoot() != null)
                    item.getRoot().accept(this, context);
            }
        }

        @Override
        public void visit(@NonNull RadiusItem item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.rotate(item.getRadius(), item.getAngle());
                } else {
                    context.rotate(item.getRadius(), item.getAngle().negate());
                    sink.next(context.copy().rotate(MINUS_PI).toPosition(item));
                }

                context.setParent(item);

                if (item.getChild() != null)
                    item.getChild().accept(this, context);

                if (!isFirst && item.getRoot() != null)
                    item.getRoot().accept(this, context);
            }
        }

        @Override
        public void visit(@NonNull SwitchLineRight item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.setParent(item);

                    if (item.getRightChild() != null) {
                        final DirectionContext context1 = context.copy().rotate(item.getRadius(), item.getAngle());
                        item.getRightChild().accept(this, context1);
                    }

                    if (item.getLeftChild() != null) {
                        context.move(item.getLineLength(), ZERO);
                        item.getLeftChild().accept(this, context);
                    }
                } else if (context.parent == item.getRightChild()) {
                    context.setParent(item);
                    context.rotate(item.getRadius(), item.getAngle().negate());
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getLeftChild() != null) {
                        context.rotate(PI).move(item.getLineLength(), ZERO);
                        item.getLeftChild().accept(this, context);
                    }
                } if (context.parent == item.getLeftChild()) {
                    context.setParent(item);
                    context.move(item.getLineLength(), ZERO);
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getRightChild() != null) {
                        context.rotate(PI).rotate(item.getRadius(), item.getAngle());
                        item.getLeftChild().accept(this, context);
                    }
                }
            }
        }

        @Override
        public void visit(@NonNull SwitchLineLeft item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.setParent(item);

                    if (item.getRightChild() != null) {
                        final DirectionContext context1 = context.copy().move(item.getLineLength(), ZERO);
                        item.getRightChild().accept(this, context1);
                    }

                    if (item.getLeftChild() != null) {
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getLeftChild().accept(this, context);
                    }
                } else if (context.parent == item.getRightChild()) {
                    context.setParent(item);

                    context.move(item.getLineLength(), ZERO);
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getRightChild() != null) {
                        context.rotate(PI).rotate(item.getRadius(), item.getAngle());
                        item.getRightChild().accept(this, context);
                    }
                } else if (context.parent == item.getLeftChild()) {
                    context.setParent(item);

                    context.rotate(item.getRadius(), item.getAngle().negate());
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getLeftChild() != null) {
                        context.rotate(PI).move(item.getLineLength(), ZERO);
                        item.getLeftChild().accept(this, context);
                    }
                }
            }
        }

        @Override
        public void visit(@NonNull SwitchR2Right item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.setParent(item);

                    if (item.getLeftChild() != null) {
                        final DirectionContext context1 = context.copy();
                        context1.move(item.getLineLength(), ZERO);
                        context1.rotate(item.getRadius(), item.getAngle());
                        item.getLeftChild().accept(this, context1);
                    }

                    if (item.getRightChild() != null) {
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getRightChild().accept(this, context);
                    }
                }
            }
        }

        @Override
        public void visit(@NonNull SwitchR2Left item, @NonNull DirectionContext context) {
            if (!context.visited(item)) {
                final boolean isFirst = context.parent == null;

                if (isFirst || context.parent == item.getRoot()) {
                    sink.next(context.toPosition(item));
                    context.setParent(item);

                    if (item.getLeftChild() != null) {
                        final DirectionContext context1 = context.copy();
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getRightChild().accept(this, context1);
                    }

                    if (item.getRightChild() != null) {
                        context.move(item.getLineLength(), ZERO);
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getLeftChild().accept(this, context);
                    }
                } else if (context.parent == item.getLeftChild()) {
                    context.setParent(item);

                    context.rotate(item.getRadius(), item.getAngle().negate());
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getRightChild() != null) {
                        context.rotate(PI);
                        context.move(item.getLineLength(), ZERO);
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getRightChild().accept(this, context);
                    }
                } if (context.parent == item.getRightChild()) {
                    context.setParent(item);
                    context.rotate(item.getRadius(), item.getAngle().negate());
                    context.move(item.getLineLength(), ZERO);
                    sink.next(context.copy().rotate(PI).toPosition(item));

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context.copy());

                    if (item.getLeftChild() != null) {
                        context.rotate(PI);
                        context.rotate(item.getRadius(), item.getAngle());
                        item.getLeftChild().accept(this, context);
                    }
                }
            }
        }
    }

    public Flux<PositionedItem> listAll() {
        return Flux.defer(() -> {
            if (rootItem == null)
                return Flux.empty();

            return Flux.create(sink -> {
                final DirectionContext context = new DirectionContext();
                final BigDecimal startAngle = PI.add(PI.divide(SIX, PRECISION)); // 150
                context.rotate(startAngle);

                rootItem.accept(new SchemeItemVisitor<>() {
                    @Override
                    public void visit(@NonNull AbstractItem item, @NonNull DirectionContext context) {
                        final ListVisitor listVisitor = new ListVisitor(sink);
                        item.accept(listVisitor, context);

                        if (item.getRoot() != null) {
                            context.clearData();
                            context.rotate(PI);
                            context.setParent(item);
                            item.getRoot().accept(listVisitor, context);
                        }
                    }
                }, context);

                sink.complete();
            });
        });
    }

    private static class Accumulator extends ContextBase {
        double value;

        public void add(double value) {
            this.value += value;
        }
    }

    private static <Context extends IContext> void iterate(@NonNull SchemeItem root,
                                                          @NonNull SchemeItemVisitor<Context> visitor,
                                                          @NonNull Context context) {
        root.accept(new SchemeItemVisitor<>() {
            @Override
            public void visit(@NonNull TwoPointItem item, @NonNull Context context) {
                if (!context.visited(item)) {
                    item.accept(visitor, context);

                    if (item.getChild() != null)
                        item.getChild().accept(this, context);

                    if (item.getRoot() != null)
                        item.getRoot().accept(this, context);
                }
            }
        }, context);
    }
}
