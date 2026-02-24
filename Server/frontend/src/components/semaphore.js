import {round} from "./utils";
import BaseCanvas from "./basecanvas";

export default class SemaphoreCanvas extends BaseCanvas {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        if (!(this.props.lightColor && nextProps.lightColor))
            return this.props.lightColor === nextProps.lightColor;

        return this.props.lightColor.r > 0 !== nextProps.lightColor.r > 0
                || this.props.lightColor.g > 0 !== nextProps.lightColor.g > 0;
    }

    dark = "rgb(120, 120, 120)";

    getYellow(ctx, centerX, centerY, radius) {
        if (this.props.lightColor && this.props.lightColor.r > 0 && this.props.lightColor.g > 0) {
            const gradient = ctx.createRadialGradient(centerX, centerY, 0, centerX, centerY, radius);
            gradient.addColorStop(0, "rgb(255, 255, 0)");
            gradient.addColorStop(0.5, "rgb(240, 240, 0)");
            gradient.addColorStop(1.0, "rgb(200, 200, 0)");
            return gradient;
        }

        return this.dark;
    }

    getRed(ctx, centerX, centerY, radius) {
        if (this.props.lightColor && this.props.lightColor.r > 0 && this.props.lightColor.g === 0) {
            const gradient = ctx.createRadialGradient(centerX, centerY, 0, centerX, centerY, radius);
            gradient.addColorStop(0, "rgb(255, 0, 0)");
            gradient.addColorStop(0.5, "rgb(240, 0, 0)");
            gradient.addColorStop(1.0, "rgb(200, 0, 0)");
            return gradient;
        }

        return this.dark;
    }

    getGreen(ctx, centerX, centerY, radius) {
        if (this.props.lightColor && this.props.lightColor.r === 0 && this.props.lightColor.g > 0) {
            const gradient = ctx.createRadialGradient(centerX, centerY, 0, centerX, centerY, radius);
            gradient.addColorStop(0, "rgb(0, 255, 0)");
            gradient.addColorStop(0.5, "rgb(0, 240, 0)");
            gradient.addColorStop(1.0, "rgb(0, 200, 0)");
            return gradient;
        }

        return this.dark;
    }

    getBounds() {
        const dpr = window.devicePixelRatio || 1;
        const bounds = super.getBounds();

        return { width: Math.floor(bounds.width / dpr), height: Math.floor(bounds.height / dpr ) };
    }

    updateCanvas() {
        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext('2d');
        const bounds = this.getBounds();

        ctx.save();

        ctx.clearRect(0, 0, bounds.width, bounds.height);
        const radius = Math.floor(Math.min(bounds.width, bounds.height) / 2) - 4;
        const centerX = bounds.width / 2;
        const centerY = bounds.height / 2;
        const lightOffset = this.props.lightOffset ? round(this.props.lightOffset) : 2;

        const isVertical = bounds.width <= bounds.height;

        ctx.lineWidth = 2;
        ctx.strokeStyle = "rgb(0, 0, 0)";
        ctx.fillStyle = "rgb(70, 70, 70)";
        ctx.beginPath();
        ctx.roundRect(0, 0, bounds.width, bounds.height, 6);
        ctx.fill();

        ctx.lineWidth = 1;
        ctx.strokeStyle = "rgb(0, 0, 0)";
        let cX, cY;

        ctx.beginPath();
        if (isVertical) {
            cX = centerX;
            cY = centerY - 2 * radius - lightOffset;
        }
        else {
            cX = centerX + 2 * radius + lightOffset;
            cY = centerY;
        }
        ctx.fillStyle = this.getRed(ctx, cX, cY, radius);
        ctx.ellipse(cX, cY, radius, radius, 0, 0, 2 * Math.PI);
        ctx.fill();

        ctx.fillStyle = this.getYellow(ctx, centerX, centerY, radius);
        ctx.beginPath();
        ctx.ellipse(centerX, centerY, radius, radius, 0, 0, 2 * Math.PI);
        ctx.fill();

        ctx.beginPath();
        if (isVertical) {
            cX = centerX;
            cY = centerY + 2 * radius + lightOffset;
        }
        else {
            cX = centerX - 2 * radius - lightOffset;
            cY = centerY;
        }
        ctx.fillStyle = this.getGreen(ctx, cX, cY, radius);
        ctx.ellipse(cX, cY, radius, radius, 0, 0, 2 * Math.PI);
        ctx.fill();

        ctx.restore();
    }
}
