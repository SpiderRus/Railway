import React from "react";
import {Rotator, round} from "./utils";

export default class TestCanvas extends React.Component {
    constructor(props) {
        super(props);

        this.canvasRef = React.createRef();

        this.init(props);
    }

    init(props) {
        this.startAngle = props.startAngle ? this.fromDegree(props.startAngle) : this.fromDegree(-20);
        this.endAngle = props.endAngle ? this.fromDegree(props.endAngle) : this.fromDegree(200);
        this.unitLabel = props.unitLabel ? props.unitLabel : "";
        this.maxSpeed = props.maxSpeed ? props.maxSpeed : 100;
        this.currentSpeed = props.speed ? props.speed : 0;
        this.centerOffsetX = props.centerOffsetX ? props.centerOffsetX : 50;
        this.centerOffsetY = props.centerOffsetY ? props.centerOffsetY : 50;
        this.bigLabelK = round(props.bigLabelK ? props.bigLabelK : (this.maxSpeed / 5));
        this.middleLabelK = round(props.middleLabelK ? props.middleLabelK : (this.maxSpeed / 10));
        this.smallLabelK = round(props.smallLabelK ? props.smallLabelK : 1);
        this.unitMultiplier = round(props.unitMultiplier ? props.unitMultiplier : 1);
        this.showSmallestLines = props.showSmallestLines ? props.showSmallestLines : true;
        this.valueStep = props.valueStep ? Math.abs(props.valueStep) : 1;
        this.radius = props.radius;

        this.lineLengthK = 0.07;
        this.arrowWidthK = 0.035;

        if (props.theme && props.theme === "dark") {
            this.backStyle = "rgb(186, 250, 230)";
            this.labelStyle = "rgb(255, 250, 0)";
        } else {
            this.backStyle = "rgb(0, 0, 0)";
            this.labelStyle = "rgb(90, 90, 90)";
        }
    }

    componentDidMount() {
        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext('2d');
        const dpr = window.devicePixelRatio || 1;
        const bounds = canvas.getBoundingClientRect();
        canvas.width = bounds.width * dpr;
        canvas.height = bounds.height * dpr;
        ctx.scale(dpr, dpr);
        canvas.style.width = `${bounds.width}px`;
        canvas.style.height = `${bounds.height}px`;

        this.updateCanvas();
    }

    getTarget() {
        return this.props.speed ? this.props.speed : 0;
    }

    startAnimate() {
        if (this.animateId)
            window.cancelAnimationFrame(this.animateId);

        this.prevAnimateTimestamp = undefined;
        this.animateId = window.requestAnimationFrame(this.animateSpeed);
    }

    componentWillUnmount() {
        if (this.animateId) {
            window.cancelAnimationFrame(this.animateId);
            this.animateId = undefined;
        }
    }

    isParametersChanged(oldProps, newProps) {
        return oldProps.maxSpeed !== newProps.maxSpeed
                || oldProps.valueStep !== newProps.valueStep
                || oldProps.startAngle !== newProps.startAngle
                || oldProps.endAngle !== newProps.endAngle
                || oldProps.unitLabel !== newProps.unitLabel

                || oldProps.width !== newProps.width
                || oldProps.height !== newProps.height
            ;
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        this.isAllChenged = this.isParametersChanged(this.props, nextProps);

        return this.isAllChenged || this.props.speed !== nextProps.speed;
    }

    componentDidUpdate(prevProps, prevState, snapshot) {
        // console.log("Update: " + this.props.speed);

        if (this.isAllChenged) {
            this.init(this.props);
            this.updateCanvas();
        } else {
            const oldSpeed = prevProps.speed ? prevProps.speed : 0;
            if (Math.abs(oldSpeed - this.getTarget()) >= this.valueStep) {
                this.smoothStep = (this.props.speed < oldSpeed ? -this.maxSpeed : this.maxSpeed) / 1000.0;
                this.startAnimate();
            }
        }
    }

    fromDegree(angle) {
        return (angle * Math.PI) / 180.0;
    }

    getTextBounds(ctx, text) {
        const metrics = ctx.measureText(text);
        // const fontHeight = metrics.fontBoundingBoxAscent + metrics.fontBoundingBoxDescent;
        const actualHeight = metrics.actualBoundingBoxAscent + metrics.actualBoundingBoxDescent;

        return { w: metrics.width, h: actualHeight };
    }

    adjustFontSize(ctx, amount) {
        const match = /(?<value>\d+\.?\d*)/;
        const newSize = round(Math.max(6, parseFloat(ctx.font.match(match).groups.value) + amount));

        return ctx.font.replace(match, newSize);
    }

    setFontSize(ctx, amount) {
        const match = /(?<value>\d+\.?\d*)/;
        return ctx.font.replace(match,  round(Math.max(6, amount)));
    }

    animateSpeed = (timestamp) => {
        if (Math.abs(this.getTarget() - this.currentSpeed) < this.valueStep) {
            this.animateId = undefined;
            return;
        }

        if (this.prevAnimateTimestamp) {
            const val = this.currentSpeed + this.smoothStep * (timestamp - this.prevAnimateTimestamp);

            this.currentSpeed = this.smoothStep < 0 ? Math.max(val, this.getTarget()) : Math.min(val, this.getTarget());

            this.updateCanvas();
        }

        this.prevAnimateTimestamp = timestamp;
        this.animateId = window.requestAnimationFrame(this.animateSpeed);
    }

    drawClockFace(context) {
        const ctx = context.ctx;

        const steps = round(this.maxSpeed / this.valueStep);
        const fullAngle = this.endAngle - this.startAngle;
        const step = fullAngle / steps;
        const lineLength = this.lineLengthK * context.length;
        const maxTextBounds = this.getTextBounds(ctx, "199");
        const labelOffset = Math.max(maxTextBounds.w, maxTextBounds.h);

        ctx.save();
        let angle = this.startAngle - Math.PI;
        ctx.fillStyle = ctx.strokeStyle = this.backStyle;

        for (let i = 0; i <= steps; i++) {
            if ((i % this.bigLabelK) < this.valueStep) {
                const rotator = new Rotator(ctx, angle);
                const xOffset = context.length - lineLength / 3;

                ctx.beginPath();
                rotator.moveTo(context.length, 1.2);
                rotator.lineTo(xOffset, 1.2);
                rotator.lineTo(context.length - lineLength, 0);
                rotator.lineTo(xOffset, -1.2);
                rotator.lineTo(context.length, -1.2);
                ctx.fill();

                if (!context.isExtraSmall) {
                    const label = "" + round(this.valueStep * i);
                    const textBounds = this.getTextBounds(ctx, label);
                    const offset = context.length - lineLength - labelOffset / 3 - (label.length - 1) * labelOffset / 8;
                    const textPoint = rotator.rotate(offset, 0);

                    ctx.fillText(label, textPoint.x - textBounds.w / 2, textPoint.y + textBounds.h / 2);
                }
            } else if ((i % this.middleLabelK) < this.valueStep) {
                const rotator = new Rotator(ctx, angle);

                ctx.beginPath();
                rotator.moveTo(context.length, 0.7);
                rotator.lineTo(context.length - 2 * lineLength / 3, 0.7);
                rotator.lineTo(context.length - 2 * lineLength / 3, -0.7);
                rotator.lineTo(context.length, -0.7);
                rotator.lineTo(context.length, 0.7);
                ctx.fill();
            } else if (context.length > 60 && this.showSmallestLines && i > 10 && (i % this.smallLabelK) < this.valueStep) {
                const rotator = new Rotator(ctx, angle);

                rotator.moveTo(context.length, 0);
                rotator.lineTo(context.length - lineLength / 3, 0);
                ctx.stroke();
            }

            angle += step;
        }
        ctx.restore();
    }

    drawCircle(ctx, w, h) {
        let length = Math.min(w, h);

        const isSmall = length <= 45;
        const isExtraSmall = length <= 20;

        const steps = this.maxSpeed;
        const fullAngle = this.endAngle - this.startAngle;
        const step = fullAngle / steps;
        const lineLength = this.lineLengthK * length;

        const adjustFont = 24 * (length / 100);

        if (isSmall) {
            if (isExtraSmall)
                ctx.font = this.setFontSize(ctx, adjustFont);
            else
                ctx.font = "italic " + this.setFontSize(ctx, adjustFont);
        } else
            ctx.font = "bold italic " + this.setFontSize(ctx, adjustFont);

        let gradient;

        ctx.save();
        const startAngle = this.startAngle - Math.PI;
        ctx.strokeStyle = "rgb(160, 20, 20)";
        const borderWidth = isSmall ? (isExtraSmall ? 2 : 4) : 6;
        length -= borderWidth;
        ctx.lineWidth = isSmall ? (isExtraSmall ? 1 : 1.4) : 2.2;
        const offsetAngle = this.fromDegree(8);
        new Rotator(ctx, startAngle - offsetAngle).moveTo(length + (borderWidth / 2), 0);
        ctx.arc(0, 0, length + (borderWidth / 2), startAngle - offsetAngle, startAngle + step * steps + offsetAngle);
        ctx.stroke();
        ctx.restore();

        const context = { ctx: ctx, length: length, isSmall: isSmall, isExtraSmall: isSmall };
        this.drawClockFace(context);

        const arrowWidth = length * this.arrowWidthK;
        const radius = arrowWidth * 3;

        if (isSmall) {
            if (!isExtraSmall) {
                ctx.save();
                ctx.fillStyle = this.labelStyle;
                ctx.font = this.adjustFontSize(ctx, 1).replace("italic", "");
                const textSpeed = "" + round(this.unitMultiplier * this.currentSpeed );
                const textBounds = this.getTextBounds(ctx, textSpeed);
                const tOffset = radius + (length - lineLength) / 2 + textBounds.h / 4;
                ctx.fillText(textSpeed, -textBounds.w / 2, tOffset);
                ctx.restore();
            }
        } else {
            ctx.save();
            ctx.fillStyle = this.labelStyle;
            ctx.font = ctx.font.replace("italic", "");
            const textSpeed = "" + round(this.unitMultiplier * this.currentSpeed);
            const textBounds = this.getTextBounds(ctx, textSpeed);
            const tOffset = radius + (length - lineLength) / 2.4;

            if (!this.unitLabel || this.unitLabel.length === 0)
                ctx.fillText(textSpeed, -textBounds.w / 2, tOffset + textBounds.h / 4);
            else {
                ctx.fillText(textSpeed, -textBounds.w / 2, tOffset + 1);

                ctx.font = this.adjustFontSize(ctx, -10 * length / 100);
                const textBounds1 = this.getTextBounds(ctx, this.unitLabel);
                ctx.fillText(this.unitLabel, -textBounds1.w / 2, tOffset + textBounds.h + 2);
            }
            ctx.restore();
        }

        // arrow
        ctx.save();
        ctx.fillStyle = "rgb(250, 50, 20)";
        ctx.strokeStyle = "rgb(250, 50, 20)";

        const rotator = new Rotator(ctx, startAngle + (fullAngle * this.currentSpeed) / this.maxSpeed);
        const tileSize = 6 * arrowWidth;
        if (length <= 60) {
            ctx.beginPath();
            if (isExtraSmall) {
                ctx.lineWidth = 1;
                rotator.moveTo(length, 0);
                rotator.lineTo(-tileSize, 0);
            } else {
                ctx.lineWidth = 2;
                rotator.moveTo(length, 0);
                rotator.lineTo(-tileSize, 0);
            }
            ctx.stroke();
        } else {
            ctx.beginPath();

            ctx.lineWidth = 1;
            rotator.moveTo(length - 1.5, 1.5);
            rotator.lineTo(-tileSize, arrowWidth);
            const point1 = rotator.rotate(-tileSize - arrowWidth, 0);
            const point2 = rotator.rotate(-tileSize, -arrowWidth);
            ctx.arcTo(point1.x, point1.y, point2.x, point2.y, arrowWidth);

            rotator.lineTo(-tileSize, -arrowWidth);
            rotator.lineTo(length - 1.5, -1.5);
            rotator.lineTo(length, 0);
            rotator.lineTo(length - 1.5, 1.5);
            ctx.fill();
        }
        ctx.restore();

        ctx.save();

        // circle
        const ofs = -length / 8;
        ctx.lineWidth = 1;
        ctx.strokeStyle = "rgb(0, 0, 0)";
        gradient = ctx.createRadialGradient(ofs, ofs, 0, ofs, ofs, -2 * ofs);
        gradient.addColorStop(0, "rgb(120, 120, 120)");
        gradient.addColorStop(1.0, "rgb(35, 35, 35)");
        ctx.fillStyle = gradient;

        ctx.beginPath();
        ctx.arc(0,0, radius, 0, 2 * Math.PI);
        ctx.fill();

        ctx.restore();
    }

    updateCanvas() {
        const canvas = this.canvasRef.current;

        if (!canvas)
            return;

        const ctx = canvas.getContext('2d');
        const bounds = canvas.getBoundingClientRect();

        ctx.save();

        ctx.fillStyle = this.props.theme && this.props.theme === "dark" ? "rgb(10, 10, 10)" : "rgb(255, 255, 255)";
        ctx.fillRect(0,0, bounds.width, bounds.height);

        const offsetX = round(bounds.width * this.centerOffsetX / 100.0);
        const offsetY = round(bounds.height * this.centerOffsetY / 100.0);

        ctx.translate(offsetX, offsetY);
        this.drawCircle(ctx, round(this.radius ? this.radius : (bounds.width / 2)),
                            round(this.radius ? this.radius : (bounds.height / 2)));

        ctx.restore();
    }

    render() {
        return <canvas width={this.props.width} height={this.props.height}
                       style={this.props.style} className={this.props.className} ref={this.canvasRef}/>;
    }
}