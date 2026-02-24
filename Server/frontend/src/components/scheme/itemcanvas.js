import BaseCanvas from "../basecanvas";
import React from "react";

export default class ItemCanvas extends BaseCanvas {
    constructor(props) {
        super(props);
    }

    init() {
        this.schemeMap = new Map();

        for (let i = 0; i < this.props.scheme.items.length; i++) {
            const item = this.props.scheme.items[i];
            this.schemeMap[item.id] = item;
        }

        this.itemMap = new Map();

        for (let i = 0; i < this.props.items.length; i++) {
            const item = this.props.items[i];
            this.itemMap[item.id] = item;
        }
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return true;
    }

    drawSwitch(ctx, item) {
        ctx.beginPath();
        ctx.lineWidth = 2;
        ctx.strokeStyle = "rgb(200, 70, 70)";

        if (item.state) {
            ctx.moveTo(0, 16.5 / 2);
            ctx.lineTo(61, 16.5 / 2);
            ctx.moveTo(0, -16.5 / 2);
            ctx.lineTo(61, -16.5 / 2);
            ctx.translate(61, 0);
        }

        const startAngle = Math.PI / 2;
        const endAngle = startAngle - Math.PI / 6;
        ctx.moveTo(0, 16.5 / 2);
        ctx.arc(0, -422, 422 + 16.5 / 2, startAngle, endAngle, true);
        ctx.moveTo(0, -16.5 / 2);
        ctx.arc(0, -422, 422 - 16.5 / 2, startAngle, endAngle, true);

        ctx.stroke();
    }

    updateCanvas() {
        const switches = this.props.items.filter(item => item.itemType === "SWITCH");

        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext("2d");
        const bounds = this.getBounds();
        const scale = this.props.scale ? this.props.scale : 1;

        ctx.save();
        ctx.clearRect(0, 0, bounds.width, bounds.height);
        ctx.scale(scale, -scale);
        ctx.translate(this.props.scheme.offsetX, -this.props.scheme.height + this.props.scheme.offsetY);

        for (let i = 0; i < switches.length; i++) {
            const sw = switches[i];
            const swScheme = this.schemeMap[sw.id];

            if (swScheme) {
                ctx.save();

                ctx.translate(swScheme.x, swScheme.y);
                ctx.rotate(swScheme.angle);
                if (swScheme.type === "SWITCH_R2_RIGHT") {
                    this.drawSwitch(ctx, sw);
                }
                ctx.restore();
            }
        }

        ctx.restore();
    }

    render() {
        return <canvas style={this.props.style} className={this.props.className} ref={this.canvasRef}/>;
    }
}