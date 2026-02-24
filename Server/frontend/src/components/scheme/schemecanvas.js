import BaseCanvas from "../basecanvas";
import React from "react";
import "../css/components.css"

const railWidth = 1.8;
const betweenRails = 16.5;
const halfBetweenRails = betweenRails / 2;
const sleeperWidth = 2.7;
const halfPI = Math.PI / 2;
const R908 = 908;
const R422 = 422;

const styles = getComputedStyle(document.documentElement);

const colors = {
    wood: styles.getPropertyValue('--sp-wood').trim(),
    gold: styles.getPropertyValue('--sp-rail-gold').trim(),
    dark: styles.getPropertyValue('--sp-rail-dark').trim(),
    bolt: styles.getPropertyValue('--sp-bolt').trim(),
    shine: styles.getPropertyValue('--sp-shine').trim(),
    grain: styles.getPropertyValue('--sp-wood-grain').trim(),
    crack: styles.getPropertyValue('--sp-wood-crack').trim(),
    black: styles.getPropertyValue('--sp-loco-black').trim() || '#1a1a1a',
    copper: styles.getPropertyValue('--sp-loco-copper').trim() || '#b87333',
    brass: styles.getPropertyValue('--sp-loco-brass').trim() || '#d4af37',
    steam: styles.getPropertyValue('--sp-loco-steam').trim() || 'rgba(255,255,255,0.4)'
};

// Функция отрисовки текстуры дерева
function drawSleeperTexture(ctx, x, y, w, h) {
    ctx.save();
    ctx.shadowColor = 'rgba(0, 0, 0, 0.7)';
    ctx.shadowBlur = 4;
    ctx.shadowOffsetX = 1.5;
    ctx.shadowOffsetY = 1.5;

    ctx.fillStyle = colors.wood;
    ctx.fillRect(x, y, w, h);
    ctx.restore();

    // Рисуем 15-20 случайных волокон на каждой шпале
    const count = 15 + 6 * Math.random();
    for (let j = 0; j < count; j++) {
        ctx.fillStyle = Math.random() > 0.5 ? colors.grain : colors.crack;
        const lineW = Math.random() * 0.5 + 0.1;
        const lineH = Math.random() * h * 0.4;
        const lineX = x + Math.random() * (w - lineW);
        const lineY = y + Math.random() * (h - lineH);
        ctx.fillRect(lineX, lineY, lineW, lineH);
    }
}

function getRLength(radius) {
    return (2 * Math.PI * radius) / 12;
}

function drawLineSleepers(ctx, length, sleepers) {
    const step = length / sleepers;
    let offset = step / 2 - sleeperWidth / 2;
    const r = 1.2;
    for (let i = 0; i < sleepers; i++) {
        drawSleeperTexture(ctx, offset, -15, sleeperWidth, 30.0);

        const centerX = offset + sleeperWidth / 2;

        // Рисуем костыль (крепление)
        ctx.fillStyle = colors.bolt;
        ctx.beginPath();
        ctx.arc(centerX, -halfBetweenRails, r, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.arc(centerX, halfBetweenRails, r, 0, Math.PI * 2);
        ctx.fill();

        offset += step;
    }
}

// Рисуем рельсы
function drawRail(ctx, y, length) {
    ctx.save();
    ctx.filter = 'blur(1px) sepia(0.5)'; // Размытие + винтажный оттенок

    ctx.beginPath();
    ctx.strokeStyle = colors.dark;
    ctx.lineWidth = railWidth;
    ctx.moveTo(0, y);
    ctx.lineTo(length, y);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeStyle = colors.gold;
    ctx.lineWidth = railWidth - 0.6;
    ctx.moveTo(0, y);
    ctx.lineTo(length, y);
    ctx.stroke();

    ctx.restore();

    ctx.beginPath();
    ctx.strokeStyle = colors.shine;
    ctx.lineWidth = 0.5;
    ctx.moveTo(0, y);
    ctx.lineTo(length, y);
    ctx.stroke();
}

function drawLineRails(ctx, length) {
    drawRail(ctx, halfBetweenRails, length);
    drawRail(ctx, -halfBetweenRails, length);
}

function sleepersFor(ctx, angle, length, sleepers, init, step) {
    const steps = sleepers + 1;
    const stepLength = length / sleepers;
    const halfStepLength = stepLength / 2;
    const stepAngle = angle / steps;
    const halfStepAngle = stepAngle / 2;

    ctx.beginPath();
    init(stepLength, stepAngle);
    ctx.rotate(halfStepAngle);
    ctx.translate(halfStepLength, 0);
    for (let i = 0; i < steps - 2; i++) {
        step(stepLength, stepAngle);
        ctx.rotate(stepAngle);
        ctx.translate(stepLength, 0);
    }
    ctx.rotate(halfStepAngle);
    ctx.translate(halfStepLength, 0);
    step(stepLength, stepAngle);
    ctx.stroke();
}

function drawRRails(ctx, angle, length, sleepers) {
    ctx.save();
    ctx.strokeStyle = colors.dark;
    ctx.lineWidth = railWidth;
    ctx.filter = 'blur(1px) sepia(0.5)'; // Размытие + винтажный оттенок
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, halfBetweenRails),
        () => ctx.lineTo(0, halfBetweenRails));
    ctx.restore();

    ctx.save();
    ctx.strokeStyle = colors.gold;
    ctx.lineWidth = railWidth - 0.6;
    ctx.filter = 'blur(1px) sepia(0.5)'; // Размытие + винтажный оттенок
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, halfBetweenRails),
        () => ctx.lineTo(0, halfBetweenRails));
    ctx.restore();

    ctx.save();
    ctx.strokeStyle = colors.shine;
    ctx.lineWidth = 0.5;
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, halfBetweenRails),
        () => ctx.lineTo(0, halfBetweenRails));
    ctx.restore();

    ctx.save();
    ctx.strokeStyle = colors.dark;
    ctx.lineWidth = railWidth;
    ctx.filter = 'blur(1px) sepia(0.5)'; // Размытие + винтажный оттенок
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, -halfBetweenRails),
        () => ctx.lineTo(0, -halfBetweenRails));
    ctx.restore();

    ctx.save();
    ctx.strokeStyle = colors.gold;
    ctx.lineWidth = railWidth - 0.6;
    ctx.filter = 'blur(1px) sepia(0.5)'; // Размытие + винтажный оттенок
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, -halfBetweenRails),
        () => ctx.lineTo(0, -halfBetweenRails));
    ctx.restore();

    ctx.save();
    ctx.strokeStyle = colors.shine;
    ctx.lineWidth = 0.5;
    sleepersFor(ctx, angle, length, sleepers,
        () => ctx.moveTo(0, -halfBetweenRails),
        () => ctx.lineTo(0, -halfBetweenRails));

    ctx.filter = "none";
    ctx.restore();
}

function drawRSleepers(ctx, angle, length, sleepers) {
    const steps = sleepers + 1;
    const stepLength = length / sleepers;
    const halfStepLength = stepLength / 2;
    const stepAngle = angle / steps;
    const halfStepAngle = stepAngle / 2;

    ctx.save();
    ctx.rotate(halfStepAngle);
    ctx.translate(halfStepLength, 0);
    const r = 1.2;
    for (let i = 0; i < steps - 1; i++) {
        drawSleeperTexture(ctx, -sleeperWidth / 2, -15, sleeperWidth, 30);

        // Рисуем костыль (крепление)
        ctx.fillStyle = colors.bolt;
        ctx.beginPath();
        ctx.arc(0, -halfBetweenRails, r, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.arc(0, halfBetweenRails, r, 0, Math.PI * 2);
        ctx.fill();

        ctx.rotate(stepAngle);
        ctx.translate(stepLength, 0);
    }
    ctx.restore();
}

function drawSteamLocoTop(ctx, x, y, angle = 0) {
    const w = 102; // Длина паровоза
    const h = 34;  // Ширина паровоза

    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(angle);

    // 1. ТЕНЬ под паровозом (размытая линия)
    ctx.save();
    ctx.filter = 'blur(4px)';
    ctx.fillStyle = 'rgba(0,0,0,0.6)';
    ctx.fillRect(-w/2 + 2, -h/2 + 2, w, h);
    ctx.restore();

    // 2. ОСНОВНОЙ КОРПУС (Котел)
    ctx.fillStyle = colors.black;
    // Передняя площадка
    ctx.fillRect(-w/2, -h/2, 15, h);

    // Цилиндрический котел (с градиентом для объема)
    const boilerGrad = ctx.createLinearGradient(0, -h/3, 0, h/3);
    boilerGrad.addColorStop(0, '#000');
    boilerGrad.addColorStop(0.5, '#333');
    boilerGrad.addColorStop(1, '#000');
    ctx.fillStyle = boilerGrad;
    ctx.fillRect(-w/2 + 15, -h/3, 60, (h*2)/3);

    // 3. КАБИНА (Будка машиниста)
    ctx.fillStyle = colors.black;
    ctx.fillRect(w/2 - 35, -h/2, 35, h);
    // Крыша кабины с козырьком
    ctx.strokeStyle = '#444';
    ctx.strokeRect(w/2 - 35, -h/2, 35, h);

    // 4. МЕДНЫЕ ДЕТАЛИ (Паровые колпаки и трубы)
    ctx.fillStyle = colors.copper;
    // Труба (Дымовая)
    ctx.beginPath();
    ctx.arc(-w/2 + 25, 0, 4, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = colors.brass;
    ctx.stroke();

    // Сухопарник (Dome) в центре
    ctx.beginPath();
    ctx.ellipse(-w/2 + 55, 0, 6, 4, 0, 0, Math.PI * 2);
    ctx.fill();

    // 5. ПОРШНИ И БАКИ (по бокам)
    ctx.fillStyle = '#222';
    ctx.fillRect(-w/2 + 20, -h/2, 40, 4); // Левый бак
    ctx.fillRect(-w/2 + 20, h/2 - 4, 40, 4); // Правый бак

    // 6. СВЕТОВОЙ БЛИК (размытая линия вдоль котла)
    ctx.save();
    ctx.beginPath();
    ctx.moveTo(-w/2 + 15, -h/6);
    ctx.lineTo(10, -h/6);
    ctx.strokeStyle = 'rgba(255,255,255,0.15)';
    ctx.lineWidth = 2;
    ctx.filter = 'blur(1px)';
    ctx.stroke();
    ctx.restore();

    // 7. ОКНА кабины (застекленные)
    ctx.fillStyle = 'rgba(135, 206, 235, 0.3)';
    ctx.fillRect(w/2 - 30, -h/2 + 4, 10, 5);
    ctx.fillRect(w/2 - 30, h/2 - 9, 10, 5);

    ctx.restore();
}

class SchemeItem {
    constructor(schemeItem) {
        this.schemeItem = schemeItem;
    }

    draw(ctx) {
    }
}

class SchemeLine extends SchemeItem {
    constructor(schemeItem, length, sleepers) {
        super(schemeItem);

        this.length = length;
        this.sleepers = sleepers;
    }

    draw(ctx) {
        drawLineSleepers(ctx, this.length, this.sleepers);
        drawLineRails(ctx, this.length);
        drawSteamLocoTop(ctx, this.length / 2, 0, 0);
    }
}

class SchemeR extends SchemeItem {
    constructor(schemeItem, angleDegree, radius, sleepers) {
        super(schemeItem);

        this.angle = (angleDegree * Math.PI) / 180.0;
        this.length = getRLength(radius);
        this.sleepers = sleepers;
    }

    draw(ctx) {
        drawRSleepers(ctx, this.angle, this.length, this.sleepers);
        drawRRails(ctx, this.angle, this.length, this.sleepers);
        //drawSteamLocoTop(ctx, this.length / 2, 0, this.angle / 2);
    }
}

class SwitchLineRight extends SchemeItem {
    constructor(schemeItem) {
        super(schemeItem);

        this.angle = -(15 * Math.PI) / 180.0;
        this.length = 239;
        this.circleLenght = getRLength(R908) / 2;
        this.sleepers = 32;
        this.circleSleepers = 32;
    }

    draw(ctx) {
        drawRSleepers(ctx, this.angle, this.circleLenght, this.circleSleepers);
        drawLineSleepers(ctx, this.length, this.sleepers);
        drawRRails(ctx, this.angle, this.circleLenght, this.circleSleepers);
        drawLineRails(ctx, this.length, this.sleepers);
    }
}

class SwitchLineLeft extends SchemeItem {
    constructor(schemeItem) {
        super(schemeItem);

        this.angle = (15 * Math.PI) / 180.0;
        this.length = 239;
        this.circleLenght = getRLength(R908) / 2;
        this.sleepers = 32;
        this.circleSleepers = 32;
    }

    draw(ctx) {
        drawRSleepers(ctx, this.angle, this.circleLenght, this.circleSleepers);
        drawLineSleepers(ctx, this.length, this.sleepers);
        drawRRails(ctx, this.angle, this.circleLenght, this.circleSleepers);
        drawLineRails(ctx, this.length, this.sleepers);
    }
}

class SwitchR2Right extends SchemeItem {
    constructor(schemeItem) {
        super(schemeItem);

        this.angle = -(30 * Math.PI) / 180.0;
        this.length = getRLength(R422);
        this.sleepers = 30;

        this.lineLength = 61;
        this.lineSleepers = 8;
    }

    draw(ctx) {
        drawLineSleepers(ctx, this.lineLength, this.lineSleepers);

        ctx.save();
        ctx.translate(this.lineLength, 0);
        drawRSleepers(ctx, this.angle, this.length, this.sleepers);
        ctx.restore();

        drawRSleepers(ctx, this.angle, this.length, this.sleepers);

        ctx.save();
        drawLineRails(ctx, this.lineLength);
        ctx.translate(this.lineLength, 0);
        drawRRails(ctx, this.angle, this.length, this.sleepers);
        ctx.restore();

        drawRRails(ctx, this.angle, this.length, this.sleepers);
    }
}

class SwitchR2Left extends SchemeItem {
    constructor(schemeItem) {
        super(schemeItem);

        this.angle = (30 * Math.PI) / 180.0;
        this.length = getRLength(R422);
        this.sleepers = 30;

        this.lineLength = 61;
        this.lineSleepers = 8;
    }

    draw(ctx) {
        drawLineSleepers(ctx, this.lineLength, this.lineSleepers);

        ctx.save();
        ctx.translate(this.lineLength, 0);
        drawRSleepers(ctx, this.angle, this.length, this.sleepers);
        ctx.restore();

        drawRSleepers(ctx, this.angle, this.length, this.sleepers);

        ctx.save();
        drawLineRails(ctx, this.lineLength);
        ctx.translate(this.lineLength, 0);
        drawRRails(ctx, this.angle, this.length, this.sleepers);
        ctx.restore();

        drawRRails(ctx, this.angle, this.length, this.sleepers);
    }
}

class G119 extends SchemeLine {
    constructor(schemeItem) {
        super(schemeItem, 119, 16);
    }
}

class G231 extends SchemeLine {
    constructor(schemeItem) {
        super(schemeItem, 231, 31);
    }
}

class G239 extends SchemeLine {
    constructor(schemeItem) {
        super(schemeItem, 239, 32);
    }
}

class R1L extends SchemeR {
    constructor(id) {
        super(id, 30, 360, 25);
    }
}

class R1R extends SchemeR {
    constructor(id) {
        super(id, -30, 360, 25);
    }
}

class R2L extends SchemeR {
    constructor(id) {
        super(id, 30, R422, 30);
    }
}

class R2R extends SchemeR {
    constructor(schemeItem) {
        super(schemeItem, -30, R422, 30);
    }
}

class R9R extends SchemeR {
    constructor(schemeItem) {
        super(schemeItem, -15, R908, 31);

        this.length /= 2;
    }
}

class R9L extends SchemeR {
    constructor(schemeItem) {
        super(schemeItem, 15, R908, 31);

        this.length /= 2;
    }
}

export default class SchemeCanvas extends BaseCanvas {
    constructor(props) {
        super(props);
    }

    init() {
        if (this.props.scheme) {
            this.schemeWidth = this.props.scheme.width;
            this.schemeHeight = this.props.scheme.height;
            this.schemeOffsetX = this.props.scheme.offsetX;
            this.schemeOffsetY = this.props.scheme.offsetY;
            this.schemeItems = this.props.scheme.items.map(item => {
                if (item.type === "R2R")
                    return new R2R(item);
                if (item.type === "R2L")
                    return new R2L(item);
                if (item.type === "R1R")
                    return new R1R(item);
                if (item.type === "R1L")
                    return new R1L(item);
                if (item.type === "G119")
                    return new G119(item);
                if (item.type === "G231")
                    return new G231(item);
                if (item.type === "G239")
                    return new G239(item);
                if (item.type === "R9R")
                    return new R9R(item);
                if (item.type === "R9L")
                    return new R9L(item);
                if (item.type === "SWITCH_LINE_RIGHT")
                    return new SwitchLineRight(item);
                if (item.type === "SWITCH_LINE_LEFT")
                    return new SwitchLineLeft(item);
                if (item.type === "SWITCH_R2_RIGHT")
                    return new SwitchR2Right(item);
                if (item.type === "SWITCH_R2_LEFT")
                    return new SwitchR2Left(item);
                return null;
            }).filter(i => i);
        } else {
            this.schemeWidth = 0;
            this.schemeHeight = 0;
            this.schemeOffsetX = 0;
            this.schemeOffsetY = 0;
            this.schemeItems = [];
        }
    }

    componentDidMount() {
        super.componentDidMount();
    }

    componentWillUnmount() {
        super.componentWillUnmount();
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return this.props.scale !== nextProps.scale;
    }

    drawGrid(ctx, bounds, scale) {
        const gridSizeX = 150;
        const gridSizeY = 150;

        ctx.save();

        const yOffset = Math.floor((bounds.height - bounds.offsetY) * scale);
        const xOffset = Math.floor(bounds.offsetX * scale);

        ctx.lineWidth = 1;
        ctx.strokeStyle = "rgb(180, 180, 180)";
        ctx.setLineDash([1, 3]);

        ctx.beginPath();
        const xStep = gridSizeX * scale;
        for (let x = xOffset + xStep; x < bounds.width; x += xStep) {
            const xF = Math.floor(x);
            ctx.moveTo(xF, 0);
            ctx.lineTo(xF, bounds.height - 1);
        }
        for (let x = xOffset - xStep; x >= 0; x -= xStep) {
            const xF = Math.floor(x);
            ctx.moveTo(xF, 0);
            ctx.lineTo(xF, bounds.height - 1);
        }
        const yStep = gridSizeY * scale;
        for (let y = yOffset + yStep; y < bounds.height; y += yStep) {
            const yF = Math.floor(y);
            ctx.moveTo(0, yF);
            ctx.lineTo(bounds.width - 1, yF);
        }
        for (let y = yOffset - yStep; y >= 0; y -= yStep) {
            const yF = Math.floor(y);
            ctx.moveTo(0, yF);
            ctx.lineTo(bounds.width - 1, yF);
        }
        ctx.stroke();

        ctx.beginPath();
        ctx.strokeStyle = "rgb(110, 110, 110)";
        ctx.setLineDash([2, 2]);
        ctx.moveTo(xOffset, 0);
        ctx.lineTo(xOffset, bounds.height - 1);

        ctx.moveTo(0, yOffset);
        ctx.lineTo(bounds.width - 1, yOffset);

        ctx.stroke();

        ctx.restore();
    }

    updateCanvas() {
        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext("2d");
        const bounds = this.getBounds();
        const scale = this.props.scale ? this.props.scale : 1;

        ctx.save();
        ctx.clearRect(0, 0, bounds.width, bounds.height);

        this.drawGrid(ctx, { offsetX: this.schemeOffsetX, offsetY: this.schemeOffsetY,
            width: this.schemeWidth, height: this.schemeHeight }, scale);

        ctx.scale(scale, -scale);
        ctx.translate(this.schemeOffsetX, -this.schemeHeight + this.schemeOffsetY);

        for (let i = 0; i < this.schemeItems.length; i++) {
            const item = this.schemeItems[i];

            ctx.save();
            ctx.translate(item.schemeItem.x, item.schemeItem.y);
            ctx.rotate(item.schemeItem.angle);
            item.draw(ctx);
            ctx.restore();
        }

        ctx.restore();
    }

    render() {
        return <canvas style={this.props.style} className={this.props.className} ref={this.canvasRef}/>;
    }
}