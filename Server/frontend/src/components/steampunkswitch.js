import React from "react";
import "./css/components.css"

export default class SteampunkSwitch extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return this.props.active !== nextProps.active;
    }

    render() {
        // active = true означает, что путь переведен

        if (this.props.type === "left") {
            const rotation = this.props.active ? -90 : 0;

            return (
                <div className="rail-switch-box">
                    <div className="switch-dial-base">
                        <span className="dial-label" style={{ top: '5%' }}>В</span>
                        <span className="dial-label" style={{ left: '5%', top: '40%' }}>Л</span>

                        <div className="lever-arm" style={{ transform: `rotate(${rotation}deg)` }}>
                            <div className="lever-knob" />
                        </div>
                    </div>
                    {/* Лампа состояния */}
                    <div className={`switch-status-lamp ${this.props.active ? 'lamp-yellow' : 'lamp-green'}`}
                         style={{ position: 'absolute', bottom: '0.3em', right: '0.3em' }} />
                </div>
            );
        }

        const rotation = this.props.active ? 90 : 0;
        return (
            <div className="rail-switch-box">
                <div className="switch-dial-base">
                    <span className="dial-label" style={{ top: '5%' }}>В</span>
                    <span className="dial-label" style={{ right: '5%', top: '40%' }}>П</span>

                    <div className="lever-arm" style={{ transform: `rotate(${rotation}deg)` }}>
                        <div className="lever-knob" />
                    </div>
                </div>
                {/* Лампа состояния */}
                <div className={`switch-status-lamp ${this.props.active ? 'lamp-yellow' : 'lamp-green'}`}
                     style={{ position: 'absolute', bottom: '0.3em', left: '0.3em' }} />
            </div>
        );
    }
}
