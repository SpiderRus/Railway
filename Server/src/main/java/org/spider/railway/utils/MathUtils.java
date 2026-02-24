package org.spider.railway.utils;

import lombok.extern.slf4j.Slf4j;
import reactor.util.annotation.NonNull;

import java.math.BigDecimal;
import java.math.MathContext;
import java.math.RoundingMode;
import java.util.HashMap;
import java.util.Map;

@Slf4j
public class MathUtils {
    private static final String THOUSAND_PI_DIGITS = "3."
            + "1415926535897932384626433832795028841971693993751" // 1 to 49
            + "05820974944592307816406286208998628034825342117067" // 50 to 99
            + "98214808651328230664709384460955058223172535940812" // 100 to 149
            + "84811174502841027019385211055596446229489549303819" // 150 to 199
            + "64428810975665933446128475648233786783165271201909" // 200 to 249
            + "14564856692346034861045432664821339360726024914127" // 250 to 299
            + "37245870066063155881748815209209628292540917153643" // 300 to 349
            + "67892590360011330530548820466521384146951941511609" // 350 to 399
            + "43305727036575959195309218611738193261179310511854" // 400 to 449
            + "80744623799627495673518857527248912279381830119491" // 450 to 499
            + "29833673362440656643086021394946395224737190702179" // 500 to 549
            + "86094370277053921717629317675238467481846766940513" // 550 to 599
            + "20005681271452635608277857713427577896091736371787" // 600 to 649
            + "21468440901224953430146549585371050792279689258923" // 650 to 699
            + "54201995611212902196086403441815981362977477130996" // 700 to 749
            + "05187072113499999983729780499510597317328160963185" // 750 to 799
            + "95024459455346908302642522308253344685035261931188" // 800 to 849
            + "17101000313783875288658753320838142061717766914730" // 850 to 899
            + "35982534904287554687311595628638823537875937519577" // 900 to 949
            + "81857780532171226806613001927876611195909216420198" // 950 to 999
            ;

    public static final MathContext PRECISION = new MathContext(34, RoundingMode.HALF_UP);

    private static final MathContext PRECISION_2048_BITS = new MathContext(2048);

    public static final BigDecimal ZERO = BigDecimal.ZERO;

    public static final BigDecimal ONE = BigDecimal.ONE;

    public static final BigDecimal TWO = toBigDecimal(2);

    public static final BigDecimal TREE = toBigDecimal(3);

    public static final BigDecimal FOUR = toBigDecimal(4);

    public static final BigDecimal FIVE = toBigDecimal(5);

    public static final BigDecimal SIX = toBigDecimal(6);

    public static final BigDecimal SEVEN = toBigDecimal(7);

    public static final BigDecimal ELEVEN = toBigDecimal(11);

    public static final BigDecimal TWELVE = toBigDecimal(12);

    public static final BigDecimal MINUS_ONE = ONE.negate();


    public static final BigDecimal SUPER_PI = new BigDecimal(THOUSAND_PI_DIGITS, PRECISION_2048_BITS);

    public static final BigDecimal PI = SUPER_PI.round(PRECISION);

    public static final BigDecimal MINUS_PI = PI.negate();

    public static final BigDecimal HALF_PI = PI.divide(TWO, PRECISION);

    public static final BigDecimal DOUBLE_PI = PI.multiply(TWO, PRECISION);

    public static final BigDecimal MINUS_DOUBLE_PI = DOUBLE_PI.negate();

    public static final BigDecimal MAX_VALUE = toBigDecimal(Double.MAX_VALUE);

    public static final BigDecimal MIN_VALUE = toBigDecimal(Double.MIN_VALUE);

    private static final int SCALE = 10;


    private static final Map<BigDecimal, BigDecimal> COS_CACHE = new HashMap<>(25);

    private static final Map<BigDecimal, BigDecimal> SIN_CACHE = new HashMap<>(25);

    static {
        final BigDecimal SQRT3_2 = TREE.sqrt(PRECISION).divide(TWO, PRECISION);
        final BigDecimal HALF = ONE.divide(TWO, PRECISION);
        final BigDecimal SQRT3_2_N = SQRT3_2.negate();
        final BigDecimal HALF_N = HALF.negate();

        // 0
        BigDecimal key = ZERO.setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, ZERO);
        COS_CACHE.put(key, ONE);

        // 30 (-30)
        key =  PI.divide(SIX, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, HALF);
        COS_CACHE.put(key, SQRT3_2);
        key = key.negate();
        SIN_CACHE.put(key, HALF_N);
        COS_CACHE.put(key, SQRT3_2);

        // 60 (-60)
        key = PI.divide(TREE, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, SQRT3_2);
        COS_CACHE.put(key, HALF);
        key = key.negate();
        SIN_CACHE.put(key, SQRT3_2_N);
        COS_CACHE.put(key, HALF);

        // 90 (-90)
        key = HALF_PI.setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, ONE);
        COS_CACHE.put(key, ZERO);
        key = key.negate();
        SIN_CACHE.put(key, MINUS_ONE);
        COS_CACHE.put(key, ZERO);

        // 120 (-120)
        key = PI.multiply(TWO, PRECISION).divide(TREE, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, SQRT3_2);
        COS_CACHE.put(key, HALF_N);
        key = key.negate();
        SIN_CACHE.put(key, SQRT3_2_N);
        COS_CACHE.put(key, HALF_N);

        // 150 (150)
        key = PI.multiply(FIVE, PRECISION).divide(SIX, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, HALF);
        COS_CACHE.put(key, SQRT3_2_N);
        key = key.negate();
        SIN_CACHE.put(key, HALF_N);
        COS_CACHE.put(key, SQRT3_2_N);

        // 180 (-180)
        key = PI.setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, ZERO);
        COS_CACHE.put(key, MINUS_ONE);
        key = key.negate();
        SIN_CACHE.put(key, ZERO);
        COS_CACHE.put(key, MINUS_ONE);

        // 210 (-210)
        key = PI.multiply(SEVEN, PRECISION).divide(SIX, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, HALF_N);
        COS_CACHE.put(key, SQRT3_2_N);
        key = key.negate();
        SIN_CACHE.put(key, HALF);
        COS_CACHE.put(key, SQRT3_2_N);

        // 240 (-240)
        key = PI.multiply(FOUR, PRECISION).divide(TREE, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, SQRT3_2_N);
        COS_CACHE.put(key, HALF_N);
        key = key.negate();
        SIN_CACHE.put(key, SQRT3_2);
        COS_CACHE.put(key, HALF_N);

        // 270 (-270)
        key = PI.multiply(TREE, PRECISION).divide(TWO, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, MINUS_ONE);
        COS_CACHE.put(key, ZERO);
        key = key.negate();
        SIN_CACHE.put(key, ONE);
        COS_CACHE.put(key, ZERO);

        // 300 (-300)
        key = PI.multiply(FIVE, PRECISION).divide(TREE, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, SQRT3_2_N);
        COS_CACHE.put(key, HALF);
        key = key.negate();
        SIN_CACHE.put(key, SQRT3_2);
        COS_CACHE.put(key, HALF);

        // 330 (-330)
        key = PI.multiply(ELEVEN, PRECISION).divide(SIX, PRECISION).setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, HALF_N);
        COS_CACHE.put(key, SQRT3_2);
        key = key.negate();
        SIN_CACHE.put(key, HALF);
        COS_CACHE.put(key, SQRT3_2);

        // 360 (-360)
        key = DOUBLE_PI.setScale(SCALE, RoundingMode.HALF_UP);
        SIN_CACHE.put(key, ZERO);
        COS_CACHE.put(key, ONE);
        key = key.negate();
        SIN_CACHE.put(key, ZERO);
        COS_CACHE.put(key, ONE);
    }

    @NonNull
    public static BigDecimal toBigDecimal(double value) {
        return new BigDecimal(value, PRECISION);
    }

    @NonNull
    public static BigDecimal toBigDecimal(int value) {
        return new BigDecimal(value, PRECISION);
    }

    @NonNull
    public static BigDecimal toBigDecimal(long value) {
        return new BigDecimal(value, PRECISION);
    }

    public static boolean equals(@NonNull BigDecimal value1, @NonNull BigDecimal value2) {
        return value1 == value2 || value1.setScale(SCALE, RoundingMode.HALF_UP).equals(value2.setScale(SCALE, RoundingMode.HALF_UP));
    }

    @NonNull
    public static BigDecimal sin(@NonNull BigDecimal angle) {
        final BigDecimal scaledAngle = angle.setScale(SCALE, RoundingMode.HALF_UP);
        final BigDecimal fromCache = SIN_CACHE.get(scaledAngle);
        return fromCache != null ? fromCache : toBigDecimal(Math.sin(angle.doubleValue()));
    }

    @NonNull
    public static BigDecimal cos(@NonNull BigDecimal angle) {
        final BigDecimal scaledAngle = angle.setScale(SCALE, RoundingMode.HALF_UP);
        final BigDecimal fromCache = COS_CACHE.get(scaledAngle);
        return fromCache != null ? fromCache : toBigDecimal(Math.cos(angle.doubleValue()));
    }

    @NonNull
    public static BigDecimal round(@NonNull BigDecimal value) {
        return value.setScale(SCALE, RoundingMode.HALF_UP);
    }

    public record Point(double x, double y) {
    }

    public record PointE(@NonNull BigDecimal x, @NonNull BigDecimal y) {
    }

    @NonNull
    public static Point rotate(double x, double y, double angle) {
        if (angle == 0.0)
            return new Point(x, y);

        final double cos = Math.cos(angle);
        final double sin = Math.sin(angle);

        return new Point(x * cos - y * sin, x * sin + y * cos);
    }

    @NonNull
    public static PointE rotate(@NonNull BigDecimal x, @NonNull BigDecimal y, @NonNull BigDecimal angle) {
        if (angle.equals(ZERO))
            return new PointE(x, y);

        final BigDecimal cos = cos(angle);
        final BigDecimal sin = sin(angle);

        return new PointE(x.multiply(cos).subtract(y.multiply(sin)), x.multiply(sin).add(y.multiply(cos)));
    }

    public static Point rotate(double centerX, double centerY, double x, double y, double angle) {
        if (angle == 0.0)
            return new Point(x, y);

        final double cos = Math.cos(angle);
        final double sin = Math.sin(angle);
        final double dx = x - centerX;
        final double dy = y - centerY;

        return new Point(centerX + dx * cos - dy * sin, centerY + dx * sin + dy * cos);
    }

    public static PointE rotate(@NonNull BigDecimal centerX, @NonNull BigDecimal centerY,
                                @NonNull BigDecimal x, @NonNull BigDecimal y, @NonNull BigDecimal angle) {
        if (angle.equals(ZERO))
            return new PointE(x, y);

        final BigDecimal cos = cos(angle);
        final BigDecimal sin = sin(angle);
        final BigDecimal dx = x.subtract(centerX);
        final BigDecimal dy = y.subtract(centerY);

        return new PointE(centerX.add(dx.multiply(cos).subtract(dy.multiply(sin))), centerY.add(dx.multiply(sin).add(dy.multiply(cos))));
    }

    @NonNull
    public static BigDecimal getDistanceToLine(@NonNull PointE point, @NonNull PointE a, @NonNull PointE b) {
        final BigDecimal tTop = point.x().subtract(a.x()).multiply(b.x().subtract(a.x())).add(
                point.y().subtract(a.y()).multiply(b.y().subtract(a.y())));
        final BigDecimal tBottom = b.x().subtract(a.x()).pow(2).add(b.y().subtract(a.y()).pow(2));
        BigDecimal t = tTop.divide(tBottom, PRECISION);

        t = t.compareTo(ZERO) <= 0 ? ZERO : ONE;

        return a.x().subtract(point.x()).add(b.x().subtract(a.x()).multiply(t)).pow(2).add(
                    a.y().subtract(point.y()).add(b.y().subtract(a.y()).multiply(t)).pow(2)
                ).sqrt(PRECISION);
    }

    public static Point findPerpendicularIntersection(double aX, double aY, double bX, double bY,
                                        double pX, double pY) {
        // Вектор отрезка AB
        double abX = bX - aX;
        double abY = bY - aY;

        // Вектор AP
        double apX = pX - aX;
        double apY = pY - aY;

        // Вычисляем скалярное произведение векторов AP и AB
        double dotProduct = apX * abX + apY * abY;

        // Вычисляем квадрат длины отрезка AB
        double lengthSqAB = abX * abX + abY * abY;

        // Если длина отрезка AB равна 0, возвращаем координаты точки A
        if (lengthSqAB == 0)
            return new Point(aX, aY);

        // Вычисляем параметр t (проекция P на прямую AB)
        double t = dotProduct / lengthSqAB;

        // Проверяем, лежит ли точка пересечения на отрезке (0 <= t <= 1)
        if (t >= 0 && t <= 1) {
            // Точка пересечения лежит на отрезке.
            double intersectionX = aX + t * abX;
            double intersectionY = aY + t * abY;
            return new Point(intersectionX, intersectionY);
        } else if (t < 0) {
            // Точка пересечения вне отрезка, ближе к A.
            return new Point(aX, aY);
        }

        // Точка пересечения вне отрезка, ближе к B.
        return new Point(bX, bY);
    }
}
