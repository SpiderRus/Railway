export function round(value) {
    return Math.floor(value + (value < 0.0 ? -0.5 : 0.5));
}

export class Rotator {
    constructor(ctx, angle) {
        this.cos = Math.cos(angle);
        this.sin = Math.sin(angle);
        this.ctx = ctx;
    }

    static rotate(x, y, angle) {
        const cos = Math.cos(angle);
        const sin = Math.sin(angle);

        return { x: x * cos - y * sin, y: x * sin + y * cos };
    }

    rotateX(x, y) {
        return x * this.cos - y * this.sin;
    }

    rotateY(x, y) {
        return x * this.sin + y * this.cos;
    }

    rotate(x, y) {
        return { x:  this.rotateX(x, y), y: this.rotateY(x, y) };
    }

    static rotateAround(centerX, centerY, x, y, angle) {
        if (angle === 0.0)
            return { x, y };

        const cos = Math.cos(angle);
        const sin = Math.sin(angle);
        const dx = x - centerX;
        const dy = y - centerY;

        return { x: centerX + dx * cos - dy * sin, y : centerY + dx * sin + dy * cos };
    }

    moveTo(x, y) {
        this.ctx.moveTo(this.rotateX(x, y), this.rotateY(x, y));
    }

    lineTo(x, y) {
        this.ctx.lineTo(this.rotateX(x, y), this.rotateY(x, y));
    }
}
