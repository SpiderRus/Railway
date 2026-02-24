package org.spider.railway.utils;

public class ReversedPID extends PID {
    private volatile double sign;

    public ReversedPID(double kp, double kd, double ki, double min, double max, double speed) {
        super(kp, kd, ki, min, max);

        setTarget(speed);
    }

    @Override
    public void setTarget(double value) {
        this.sign = Math.signum(value);

        super.setTarget(Math.abs(value));
    }

    @Override
    public double getResult(double currentValue) {
        return sign * super.getResult(currentValue);
    }
}
