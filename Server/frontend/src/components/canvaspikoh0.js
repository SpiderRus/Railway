import BaseCanvas from "./basecanvas";
import "./css/components.css"

export default class CanvasPikoH0 extends BaseCanvas {
    constructor(props) {
        super(props);

        // Получаем цвета из CSS переменных
        const style = getComputedStyle(document.documentElement);
        this.colors = {
            wood: style.getPropertyValue('--sp-wood').trim(),
            gold: style.getPropertyValue('--sp-rail-gold').trim(),
            dark: style.getPropertyValue('--sp-rail-dark').trim(),
            bolt: style.getPropertyValue('--sp-bolt').trim(),
            shine: style.getPropertyValue('--sp-shine').trim(),
            grain: style.getPropertyValue('--sp-wood-grain').trim(),
            crack: style.getPropertyValue('--sp-wood-crack').trim()
        };
    }

    init(props) {
        super.init(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return false;
    }

    // Функция отрисовки текстуры дерева
    drawSleeperTexture(ctx, x, y, w, h) {
        ctx.fillStyle = this.colors.wood;
        ctx.fillRect(x, y, w, h);

        // Рисуем 15-20 случайных волокон на каждой шпале
        const count = 15 + 6 * Math.random();
        for (let j = 0; j < count; j++) {
            ctx.fillStyle = Math.random() > 0.5 ? this.colors.grain : this.colors.crack;
            const lineW = Math.random() * 0.5 + 0.1;
            const lineH = Math.random() * h * 0.4;
            const lineX = x + Math.random() * (w - lineW);
            const lineY = y + Math.random() * (h - lineH);
            ctx.fillRect(lineX, lineY, lineW, lineH);
        }
    }

    // Рисуем рельсы
    drawRail(ctx, y, length) {
        // Градиент металла
        const grad = ctx.createLinearGradient(0, y, 0, y + 2);
        grad.addColorStop(0, this.colors.dark);
        grad.addColorStop(0.5, this.colors.gold);
        grad.addColorStop(1, this.colors.dark);

        ctx.fillStyle = grad;
        ctx.fillRect(0, y, length, 1.8);

        // Блик
        ctx.fillStyle = this.colors.shine;
        ctx.fillRect(0, y + 0.3, length, 0.4);
    }

    updateCanvas() {
        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext('2d');
        const bounds = this.getBounds();
        const scale = this.props.scale ? this.props.scale : 6;

        ctx.save();
        ctx.clearRect(0, 0, bounds.width, bounds.height);
        ctx.scale(scale, scale);

        // Параметры G239 (H0)
        const length = 239;
        const gauge = 16.5;
        const sleeperCount = 32;
        const sleeperL = 30;
        const sleeperW = 2.5;
        const spacing = length / sleeperCount;

        const r = 1.2;
        for (let i = 0; i < sleeperCount; i++) {
            const centerX = spacing / 2 + i * spacing;

            // 1. Шпалу с текстурой
            this.drawSleeperTexture(ctx, centerX - sleeperW / 2, 0, sleeperW, sleeperL);

            // Рисуем костыль (крепление)
            ctx.fillStyle = this.colors.bolt;
            // Верхний рельс
            ctx.beginPath();
            ctx.arc(centerX, sleeperL / 2 - gauge / 2, r, 0, Math.PI * 2);
            ctx.fill();
            // Нижний рельс
            ctx.beginPath();
            ctx.arc(centerX, sleeperL / 2 + gauge / 2, r, 0, Math.PI * 2);
            ctx.fill();
        }

        // Рисуем рельсы
        this.drawRail(ctx, sleeperL / 2 - gauge / 2 - 0.9, length);
        this.drawRail(ctx, sleeperL / 2 + gauge / 2 - 0.9, length);

        ctx.restore();
    }
}
//
// const CanvasPikoH0 = () => {
//     const canvasRef = useRef(null);
//
//     useEffect(() => {
//     }, []);
//
//     return (
//         <div className="steampunk-canvas-container">
//             <canvas
//                 ref={canvasRef}
//                 width={ 2 * 239 }
//                 height={ 2 * 20 }
//                 className="rail-canvas"
//             />
//         </div>
//     );
// };
//
// export default CanvasPikoH0;
