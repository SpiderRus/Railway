import React from "react";
import "./css/components.css"

export default class SteampunkRail extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return true;
    }

    render() {
        const numSleepers = 32;
        const totalLength = 239.0;
        const sleeperLength = 30.0;
        const sleeperWidth = 2.5;
        const gauge = 16.5;
        const spacing = totalLength / (numSleepers + 1);

        return (
            <div className="rail-h0-container">
                <svg
                    width="100%"
                    height="auto"
                    viewBox={`0 0 ${totalLength} 40`}
                    className="rail-h0-steampunk"
                >
                    <defs>
                        <linearGradient id="brassH0" x1="0%" y1="0%" x2="0%" y2="100%">
                            <stop offset="0%" stopColor="#5d3a1a" />
                            <stop offset="50%" stopColor="#d4af37" />
                            <stop offset="100%" stopColor="#3d1f05" />
                        </linearGradient>
                    </defs>

                    {/* 1. Сначала рисуем только ШПАЛЫ */}
                    {Array.from({ length: numSleepers }).map((_, i) => (
                        <rect
                            key={`s-${i}`}
                            x={(i + 1) * spacing - sleeperWidth / 2}
                            y={(40 - sleeperLength) / 2}
                            width={sleeperWidth} height={sleeperLength}
                            className="sleeper-h0"
                            rx="0.3"
                        />
                    ))}

                    {/* 3. И в конце КОСТЫЛИ (поверх рельсов) */}
                    {Array.from({ length: numSleepers }).map((_, i) => {
                        const xPos = (i + 1) * spacing;
                        return (
                            <g key={`sp-${i}`}>
                                {/* Верхний рельс */}
                                <circle cx={xPos} cy={20 - gauge/2} r="1.1" className="spike-h0" />
                                {/* Нижний рельс */}
                                <circle cx={xPos} cy={20 + gauge/2} r="1.1" className="spike-h0" />
                            </g>
                        );
                    })}

                    {/* 2. Затем РЕЛЬСЫ (поверх шпал) */}
                    <rect x="0" y={20 - gauge/2 - 0.75} width={totalLength} height="1.5" className="rail-h0-metal" rx="0.4" />
                    <rect x="0" y={20 + gauge/2 - 0.75} width={totalLength} height="1.5" className="rail-h0-metal" rx="0.4" />
                    <rect x="0" y={20 - gauge/2 - 0.3} width={totalLength} height="0.4" fill="rgba(255,255,255,0.4)" />
                    <rect x="0" y={20 + gauge/2 - 0.3} width={totalLength} height="0.4" fill="rgba(255,255,255,0.4)" />

                </svg>
            </div>
        );
    }
}
