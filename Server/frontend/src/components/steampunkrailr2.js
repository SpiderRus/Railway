import React from "react";
import "./css/components.css"

export default class SteampunkRailR2 extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return true;
    }

    render() {
            const radius = 421.88;
            const angleDeg = 30;
            const numSleepers = 28;
            const gauge = 16.5;
            const sleeperLength = 30.0;
            const sleeperWidth = 2.5;

            const centerX = 110;
            const centerY = radius + 20;

            // Вылет рельса за шпалы в градусах (примерно 0.7° соответствует ~5мм при таком радиусе)
            const overhandDeg = 0.7;

            const getPoint = (r, deg) => {
                // Смещение на -90 и центрирование дуги по оси Y
                const rad = (deg - 90 - angleDeg / 2) * Math.PI / 180;
                return {
                    x: centerX + r * Math.cos(rad),
                    y: centerY + r * Math.sin(rad)
                };
            };

            const generateRailPoints = (r) => {
                const points = [];
                // Начинаем чуть раньше (-overhandDeg) и заканчиваем чуть позже (+overhandDeg)
                for (let i = -10; i <= 70; i++) {
                    const stepDeg = (i / 60) * angleDeg;
                    // Масштабируем i так, чтобы выйти за пределы 0-30 градусов
                    const currentDeg = -overhandDeg + (i / 60) * (angleDeg + 2 * overhandDeg);
                    const p = getPoint(r, currentDeg);
                    points.push(`${p.x},${p.y}`);
                }
                return points.join(' ');
            };

            const Joiner = ({ r, deg }) => {
                const p = getPoint(r, deg);
                const rotation = deg - angleDeg / 2;
                return (
                    <rect
                        x={p.x - 2} y={p.y - 1}
                        width="4" height="2"
                        className="rail-joiner"
                        rx="0.3"
                        transform={`rotate(${rotation}, ${p.x}, ${p.y})`}
                    />
                );
            };

            return (
                <div className="rail-h0-container">
                    <svg width="400" height="120" viewBox="0 0 220 50" className="rail-h0-steampunk" style={{ overflow: 'visible' }}>
                        <defs>
                            <linearGradient id="brassH0" x1="0%" y1="0%" x2="0%" y2="100%">
                                <stop offset="0%" stopColor="#5d3a1a" />
                                <stop offset="50%" stopColor="#d4af37" />
                                <stop offset="100%" stopColor="#3d1f05" />
                            </linearGradient>
                        </defs>

                        {/* 1. ШПАЛЫ (Остаются в пределах 0-30 градусов) */}
                        {Array.from({ length: numSleepers }).map((_, i) => {
                            const currentAngle = -angleDeg / 2 + (i / (numSleepers - 1)) * angleDeg;
                            return (
                                <g key={i} transform={`rotate(${currentAngle}, ${centerX}, ${centerY})`}>
                                    <rect x={centerX - sleeperWidth / 2} y={centerY - radius - sleeperLength / 2} width={sleeperWidth} height={sleeperLength} className="sleeper-h0" rx="0.3" />
                                    <circle cx={centerX} cy={centerY - radius - gauge / 2} r="1.1" className="spike-h0" />
                                    <circle cx={centerX} cy={centerY - radius + gauge / 2} r="1.1" className="spike-h0" />
                                </g>
                            );
                        })}

                        {/* 2. РЕЛЬСЫ (Теперь длиннее шпального поля) */}
                        <polyline points={generateRailPoints(radius - gauge / 2)} fill="none" stroke="url(#brassH0)" strokeWidth="1.6" />
                        <polyline points={generateRailPoints(radius + gauge / 2)} fill="none" stroke="url(#brassH0)" strokeWidth="1.6" />

                        {/* 3. СОЕДИНИТЕЛИ (Ставим на самые концы УДЛИНЕННЫХ рельсов) */}
                        <Joiner r={radius - gauge / 2} deg={-overhandDeg} />
                        <Joiner r={radius + gauge / 2} deg={-overhandDeg} />
                        <Joiner r={radius - gauge / 2} deg={angleDeg + overhandDeg} />
                        <Joiner r={radius + gauge / 2} deg={angleDeg + overhandDeg} />
                    </svg>
                </div>
            );
    }
}
