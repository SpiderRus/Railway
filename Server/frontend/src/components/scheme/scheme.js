import React from "react";
import SchemeCanvas from "./schemecanvas";
import ItemCanvas from "./itemcanvas";

export default class Scheme extends React.Component {
    constructor(props) {
        super(props);

        this.schemeCanvasRef = React.createRef();
        this.itemsCanvasRef = React.createRef();
        this.divRef = React.createRef();
        this.props.scheme.offsetX += 30;
        this.props.scheme.offsetY -= 30;

        this.state = {};
    }

    getSchemeScale() {
        const bounds = this.divRef.current.getBoundingClientRect();

        return Math.min(bounds.width / this.props.scheme.width, bounds.height / this.props.scheme.height);
    }

    onMouseWheel = event => {
        if (!this.state.scale) {
            this.loopScale = this.state.scale = this.getSchemeScale();
        }

        this.loopScale += this.loopScale * 0.0010 * -event.deltaY;

        if (Math.abs(this.state.scale - this.loopScale) >= 0.05) {
            this.state.scale = this.loopScale;
            this.setState(this.state);
        }
    };

    componentDidMount() {
        this.divRef.current.addEventListener("mousewheel", this.onMouseWheel);
    }

    componentWillUnmount() {
        this.divRef.current.removeEventListener("mousewheel", this.onMouseWheel);
    }

    render() {
        return (
            <div style={{width: "100%", height: "90vh", position: "relative", border: "1px solid black"}} ref={this.divRef}>
                <SchemeCanvas style={{width: "100%", height: "100%", left: 0, top: 0, position: "absolute", pointerEvents: "none" }}
                        ref={this.schemeCanvasRef} scheme={this.props.scheme} scale={this.state.scale}/>
                <ItemCanvas style={{width: "100%", height: "100%", left: 0, top: 0, position: "absolute", pointerEvents: "none" }}
                        scheme={this.props.scheme} items={this.props.items} scale={this.state.scale} ref={this.itemsCanvasRef}/>
            </div>
        );
    }
}