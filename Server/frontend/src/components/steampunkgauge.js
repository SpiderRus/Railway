import React from "react";
import "./css/components.css"

export default class SteampunkGauge extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return this.props.value !== nextProps.value
                || this.props.min !== nextProps.min
                || this.props.max !== nextProps.max;
    }

    render() {
            const percent = Math.min(Math.max(this.props.value, this.props.min), this.props.max) / (this.props.max - this.props.min);
            const rotation = -120 + (percent * 240);
            const isOverheat = this.props.value >= this.props.max * 0.9; // Перегрев выше 90%

            const interval = 20;

            // Массив меток через каждые 20 единиц
            const marks = [];
            for (let i = this.props.min; i <= this.props.max; i += interval) {
                const labelPercent = (i - this.props.min) / (this.props.max - this.props.min);
                // Угол в радианах (смещение на -120 градусов от вертикали)
                const angle = ((-120 + (labelPercent * 240)) - 90) * (Math.PI / 180);

                // ПРОВЕРКА: находится ли стрелка рядом с этой цифрой (радиус захвата ±10 единиц)
                const isActive = Math.abs(this.props.value - i) <= 10;

                // Точки чуть дальше (радиус 40), цифры чуть ближе (радиус 32)
                marks.push({
                    val: i,
                    active: isActive,
                    dotX: 50 + 41 * Math.cos(angle),
                    dotY: 50 + 41 * Math.sin(angle),
                    lblX: 50 + 25 * Math.cos(angle),
                    lblY: 50 + 25 * Math.sin(angle)
                });
            }

            return (
                <div className={`steampunk-gauge ${isOverheat ? 'overheat' : ''}`}>
                    <div className="gauge-ring" />

                    {/* Слой динамической подсветки шкалы */}
                    <div
                        className="gauge-light-overlay"
                        style={{ transform: `rotate(${rotation}deg)` }}
                    >
                    <div className="gauge-spotlight" />
                    </div>

                    <div className="gauge-glass" />

                    {/* Мелкие точки оставляем в SVG (они для декора) */}
                    <svg className="gauge-ticks" viewBox="0 0 100 100">
                        <path
                            className="ticks-minor"
                            d="M 21.4 78.6 A 41 41 0 1 1 78.6 78.6"
                            strokeDasharray="0.1 6.7" /* Автоматический подбор шага */
                        />
                    </svg>

                    {/* Крупные точки — теперь они всегда под цифрами */}
                    <div className="gauge-major-dots">
                        {marks.map((m, i) => (
                            <div key={i} className="major-dot" style={{ left: `${m.dotX}%`, top: `${m.dotY}%` }} />
                        ))}
                    </div>

                    {/* Цифры */}
                    <div className="gauge-labels">
                        {marks.map((m, i) => (
                            <div key={i} className={`gauge-label ${m.active ? 'active' : ''}`}
                                 style={{ left: `${m.lblX}%`, top: `${m.lblY}%` }}>
                                {m.val}
                            </div>
                        ))}
                    </div>

                    {/*<div className="gauge-odometer">*/}
                    {/*    {mileageDigits.map((digit, i) => (*/}
                    {/*        <div key={i} className="odometer-digit">*/}
                    {/*            <div className="digit-strip" style={{ transform: `translateY(-${digit * 1.2}em)` }}>*/}
                    {/*                {digitsArray.map(n => <div key={n} className="digit-number">{n}</div>)}*/}
                    {/*            </div>*/}
                    {/*        </div>*/}
                    {/*    ))}*/}
                    {/*</div>*/}

                    <div className="gauge-needle" style={{
                        '--angle': `${rotation}deg`,
                        transform: `translateX(-50%) rotate(${rotation}deg)` }} />
                    <div className="gauge-center" />
                    {/*<div className="gauge-plate">KM/H</div>*/}
                </div>
            );
    }
}
