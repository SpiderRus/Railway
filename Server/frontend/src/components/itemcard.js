import Card from "react-bootstrap/Card";
import React from "react";
import PropertiesGrid from "./propertiesgrid";
import {outDfFormatter} from "./dateutils";
import TestCanvas from "./testcanvas";
import SemaphoreCanvas from "./semaphore.js"
import '../App.css';
import SteampunkSemaphore from "./steampunksemaphore";
import SteampunkGauge from "./steampunkgauge";
import SteampunkSwitch from "./steampunkswitch";
import SteampunkRail from "./steampunkrail";

function getTypeName(item) {
    switch (item.type) {
        case "TRAIN": return "Локомотив";
        case "SWITCH": return "Стрелка";
        case "SEMAPHORE": return "Семафор";
    }

    return "Unknown";
}

export default class ItemCard extends React.Component {
    constructor(props) {
        super(props);

        this.prevGaugeValue = 0;
    }

    componentDidMount() {
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return nextProps.item.internalTemp !== this.props.item.internalTemp
                || nextProps.item.lastTimestamp !== this.props.item.lastTimestamp
                || ( this.props.item.type === "TRAIN" && nextProps.item.speed !== this.props.item.speed )
                || nextProps.item.id !== this.props.item.id;
    }

    // componentWillUnmount(){
    // }
    //
    // componentDidUpdate(prevProps, prevState, snapshot) {
    // }

    getTempLink(pair) {
        return <a href={ "../temperature_graph.html?item=" + pair.name } target="_blank">{ pair.name }</a>;
    }

    getSpeedLink(pair) {
        return <a href={ "../speed_graph.html?item=" + pair.name } target="_blank">{ pair.name }</a>;
    }

    getSpeedValue(pair) {
        return  <>
                    <TestCanvas width="68px" height="78px" unitLabel="x1000"
                                centerOffsetX={71} centerOffsetY={60} radius={47}
                                startAngle={-35} endAngle={110} maxSpeed={10} unitMultiplier={1000}
                                showSmallestLines={false} bigLabelK={20} middleLabelK={10} valueStep={0.1}/>
                    <TestCanvas width="68px" height="78px" unitLabel="mm/s"
                                centerOffsetX={29} centerOffsetY={60} radius={47}
                                startAngle={70} endAngle={215} maxSpeed={100} speed={ pair.value }/>
                </>;
    }

    getSemaphoreValue(pair) {
        return <SemaphoreCanvas style={{ marginLeft: "57px" }} lightColor={ pair.value } width="27px" height="78px" lightOffset={4}/>
    }

    getSemaphoreColor() {
        const rnd = 3 * Math.random();

        return rnd < 1 ? { r: 0, g: 255, b: 0 }
                    : ( rnd < 2 ? { r: 255, g: 255, b: 0 } : { r: 255, g: 0, b: 0 });
    }

    getSteampunkSemaphoreValue() {
        const rnd = 3 * Math.random();
        return rnd < 1 ? "go" : (rnd < 2 ? "stop" : "warning");
    }

    getSteampunkSemaphore(pair) {
        return <div style={{ fontSize: '7px', display: 'flex', justifyContent: 'center' }}><SteampunkSemaphore status={pair.value} /></div>;
    }

    getSteampunkGaugeValue() {
        let result = this.prevGaugeValue + 12 * Math.random() - 5;

        if (result < 0)
            result = -result;

        if (result > 100)
            result = 0;

        return this.prevGaugeValue = result;
    }

    getSteampunkGauge(pair) {
        return <div style={{ fontSize: '19px', display: 'flex', justifyContent: 'center' }}><SteampunkGauge value={pair.value} min={0} max={100} mileage={0}/></div>;
    }

    getSteampunkSwitchValue() {
        const rnd = 2 * Math.random();
        return rnd < 1;
    }

    getSteampunkSwitchLeft(pair) {
        return <div style={{ fontSize: '12px', display: 'flex', justifyContent: 'center' }}><SteampunkSwitch type="left" active={pair.value} /></div>;
    }

    getSteampunkSwitchRight(pair) {
        return <div style={{ fontSize: '12px', display: 'flex', justifyContent: 'center' }}><SteampunkSwitch type="right" active={pair.value} /></div>;
    }

    getSteampunkRail(pair) {
        return <div style={{ fontSize: '20px', display: 'flex', justifyContent: 'center' }}><SteampunkRail/></div>;
    }

    render() {
        return (
            <Card style={{ width: '24rem' }} className="steampunk-card">
                {/*<Card.Header>*/}
                {/*    { "Header: " + this.props.item.id }*/}
                {/*</Card.Header>*/}
                <Card.Body style={{ textAlign: 'left' }}>
                    <Card.Title>{ this.props.item.id }</Card.Title>
                    <Card.Subtitle className="mb-2 text-muted">{ getTypeName(this.props.item) }</Card.Subtitle>
                    <PropertiesGrid style={{ fontSize: 'smaller' }} items={ [
                        { name: "Internal temp", value: this.props.item.internalTemp, nameGetter: this.getTempLink, id: this.props.item.id },
                        { name: "Timestamp", value: outDfFormatter.format(new Date(this.props.item.lastTimestamp)) },
                        this.props.item.type === "TRAIN" ? { name: "Speed", value: this.props.item.speed,
                                                            nameGetter: this.getSpeedLink, valueGetter: this.getSpeedValue } : null,
                        this.props.item.type === "SEMAPHORE" ? { name: "Color", value: this.getSemaphoreColor(),
                                                valueGetter: this.getSemaphoreValue } : null,
                        { name: "Semaphore", value: this.getSteampunkSemaphoreValue(), valueGetter: this.getSteampunkSemaphore },
                        { name: "Gauge", value: this.getSteampunkGaugeValue(), valueGetter: this.getSteampunkGauge },
                        { name: "SwitchL", value: this.getSteampunkSwitchValue(), valueGetter: this.getSteampunkSwitchLeft },
                        { name: "SwitchR", value: this.getSteampunkSwitchValue(), valueGetter: this.getSteampunkSwitchRight },
                        { name: "Rail", value: 0, valueGetter: this.getSteampunkRail }
                    ].filter(v => v) } />
                    {/*<Card.Link href="#">Card Link</Card.Link>*/}
                    {/*<Card.Link href="#">Another Link</Card.Link>*/}
                </Card.Body>
            </Card>
        );
    }
}
