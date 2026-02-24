import {Table} from "react-bootstrap";
import React from "react";
import "./components.css";
import "../App.css"

class Property extends React.Component {
    constructor(props) {
        super(props);
    }

    shouldComponentUpdate(nextProps, nextState, nextContext) {
        return this.props.pair.value !== nextProps.pair.value || this.props.pair.name !== nextProps.pair.name;
    }

    render() {
        return (<tr>
            <td>{ this.props.pair.nameGetter ? this.props.pair.nameGetter(this.props.pair) : (<span>{this.props.pair.name}</span>) }</td>
            <td>{ this.props.pair.valueGetter ? this.props.pair.valueGetter(this.props.pair) : (<span>{this.props.pair.value}</span>) }</td>
        </tr>);
    }
}

export default class PropertiesGrid extends React.Component {
    constructor(props) {
        super(props);
    }

    render() {
        return (
            <Table style={this.props.style} bordered responsive className="steampunk-table steampunk-table-hover steampunk-table-striped">
                <tbody>{ this.props.items.map(pair => (<Property pair={pair}/>) )}</tbody>
            </Table>
        );
        // return <Form style={this.props.style}>{ this.props.items.map(pair => (<Property pair={pair}/>) )}</Form>;
    }
}
