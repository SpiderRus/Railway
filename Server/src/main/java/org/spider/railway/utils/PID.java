package org.spider.railway.utils;

import lombok.extern.slf4j.Slf4j;

@Slf4j
public class PID {
    private final double Kp, Kd, Ki, min, max;
    private double prev, integral;
    private double target;
    private long prevMillis;

    public PID(double kp, double kd, double ki, double min, double max) {
        this.Kp = kp;
        this.Kd = kd;
        this.Ki = ki;
        this.min = min;
        this.max = max;

        this.prev = 0.0;
        this.prevMillis = 0;
        this.integral = 0.0;
        this.target = 0.0;
    }

    private double constraint(double value) {
        return Math.max(min, Math.min(value, max));
    }

    public synchronized void setTarget(double target) {
        this.target = target;
    }

    public synchronized double getResult(double current) {
        final double error = target - current;
        final long currentMillis = System.currentTimeMillis();

        //double prop = error * this.Kp, diff = 0.0, integr = 0.0;
        double result = error * this.Kp; // proportional

        if (prevMillis > 0) {
            final double delta = this.prev - current;
            final double deltaSec = (currentMillis - prevMillis) / 1000.0;
            //diff = delta * this.Kd / deltaSec;
            result += delta * this.Kd / deltaSec; // differential

            //integr = constraint(integral + error * Ki * deltaSec);
            this.integral = constraint(integral + error * Ki * deltaSec);
            result += integral;
        }

        //log.info("Prop={}, Diff={}, Integr={}.", prop, diff, integr);

        this.prevMillis = currentMillis;
        this.prev = current;

        return constraint(result);
    }
}
