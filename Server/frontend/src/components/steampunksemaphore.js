import React from "react";
import "./css/components.css"

export default class SteampunkSemaphore extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return this.props.status !== nextProps.status;
    }

    render() {
        // status может быть: 'stop' (красный), 'warning' (желтый), 'go' (зеленый)

        return (
            <div className="steampunk-semaphore">
                {/* Красный */}
                <div className="lens-container">
                    <div className="lens-glass" />
                    <div className={`light red ${this.props.status === 'stop' ? 'active' : ''}`} />
                </div>

                {/* Желтый */}
                <div className="lens-container">
                    <div className="lens-glass" />
                    <div className={`light yellow ${this.props.status === 'warning' ? 'active' : ''}`} />
                </div>

                {/* Зеленый */}
                <div className="lens-container">
                    <div className="lens-glass" />
                    <div className={`light green ${this.props.status === 'go' ? 'active' : ''}`} />
                </div>
            </div>
        );
    }
}
