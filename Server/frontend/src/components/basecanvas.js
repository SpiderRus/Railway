import React from "react";

const scaleMultiplier = 1;

export default class BaseCanvas extends React.Component {
    constructor(props) {
        super(props);

        this.updateInProgress = false;

        this.canvasRef = React.createRef();

        this.init(props);
    }

    init(props) {
    }

    onResize = () => {
        const canvas = this.canvasRef.current;

        if (canvas) {
            const bounds = canvas.getBoundingClientRect();

            // if (canvas.width !== bounds.width || canvas.height !== bounds.height) {
            //     canvas.width = bounds.width;
            //     canvas.height = bounds.height;
            //
            //     this.updateCanvas();
            // }
            const width = Math.floor(scaleMultiplier * bounds.width);
            const height = Math.floor(scaleMultiplier * bounds.height);
            if (canvas.width !== width || canvas.height !== height) {
                canvas.width = width;
                canvas.height = height;

                this.updateCanvas();
            }
        }
    }

    getBounds() {
        if (scaleMultiplier === 1)
            return this.canvasRef.current.getBoundingClientRect();

        const dpr = window.devicePixelRatio || 1;
        const canvas = this.canvasRef.current;
        return { width: Math.floor(canvas.width / dpr), height: Math.floor(canvas.height / dpr) };
    }

    componentDidMount() {
        const canvas = this.canvasRef.current;
        const ctx = canvas.getContext('2d');
        const bounds = canvas.getBoundingClientRect();

        const dpr = window.devicePixelRatio || 1;
        canvas.width = Math.floor( scaleMultiplier * bounds.width * dpr);
        canvas.height = Math.floor(scaleMultiplier * bounds.height * dpr);
        ctx.scale(dpr, dpr);

        this.updateCanvas();

        window.addEventListener("resize", this.onResize);
    }

    componentWillUnmount() {
        window.removeEventListener("resize", this.onResize);
    }

    componentDidUpdate(prevProps, prevState, snapshot) {
        if (!this.updateInProgress)
            requestAnimationFrame(() => {
                try {
                    this.updateCanvas();
                } finally {
                    this.updateInProgress = false;
                }
            })
    }

    updateCanvas() {
    }

    render() {
        return <canvas width={this.props.width} height={this.props.height}
                       style={this.props.style} className={this.props.className} ref={this.canvasRef}/>;
    }
}