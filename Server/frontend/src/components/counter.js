import 'bootstrap/dist/css/bootstrap.min.css';
import '../App.css';

import React, {useEffect, useState} from 'react';
import Nav from 'react-bootstrap/Nav';
import Container from 'react-bootstrap/Container';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';
import axios from 'axios';
import ItemCard from "./itemcard"
import {Accordion, Alert} from "react-bootstrap";
import Scheme from "./scheme/scheme";
import SteampunkRail from "./steampunkrail";
import SteampunkRailR2 from "./steampunkrailr2";
import CanvasPikoH0 from "./canvaspikoh0";

function compareArrays(a, b, comparator) {
    return a.length === b.length && a.every((element, index) => comparator(element, b[index]));
}

function getGrid(items) {
    const now = Date.now();
    return (
        <Container fluid>
            <Row>
                { items.map(item => <Col className="Item-row-cards"><ItemCard item={
                    { id: item.id,
                        type: item.itemType,
                        internalTemp: item.internalTemp,
                        lastTimestamp: now,
                        speed: 100 * Math.random()
                    }
                }></ItemCard></Col>) }
            </Row>
        </Container>
    );
}

function groupByKey(list, key) {
    return list.reduce((map, obj) => {
        const group = obj[key];
        const val = map.get(group);

        if (val === undefined)
            map.set(group, [obj]);
        else
            val.push(obj);

        return map;
    }, new Map());
}

function getAccordion(items) {
    return groupByKey(items, 'type').entries().map(([key, value]) => {
        return <Accordion.Item eventKey={"GR_" + key}>
            <Accordion.Header className="no-borders">{key}</Accordion.Header>
            <Accordion.Body>Body 1</Accordion.Body>
        </Accordion.Item>;
    }).toArray();
}

export default function useCounter() {
    const [items, setItems] = useState([]);
    const [scheme, setScheme] = useState([]);
    const [activeTab, setActiveTab] = useState("item_cards")

    useEffect(() => {
        function update() {
                axios.get("/api/scheme-items/all")
                    .then(response => {
                        setItems(response.data);
                    })
                    .catch(error => {
                        setItems([]);
                    });
            }

        function updateScheme() {
            axios.get("/api/scheme")
                .then(response => {

                    setScheme(response.data);
                })
                .catch(error => {
                    setScheme(null);
                });
        }

        const intervalId = setInterval(update, 1_000);
        update();
        updateScheme();

        return () => {
            clearInterval(intervalId);
        }
    }, []);

    return (
        <>
            <Nav variant="underline" activeKey={ activeTab } className="steampunk-nav justify-content-center" style={{ marginBottom: "10px" }}
                 onSelect={ key => setActiveTab(key) }>
                <Nav.Item>
                    <Nav.Link eventKey="item_cards">Item cards</Nav.Link>
                </Nav.Item>
                <Nav.Item>
                    <Nav.Link eventKey="scheme">Scheme</Nav.Link>
                </Nav.Item>
                <Nav.Item>
                    <Nav.Link eventKey="graphs">Graphs</Nav.Link>
                </Nav.Item>
            </Nav>
            { activeTab === "scheme" && <Scheme scheme={scheme} items={items}/> }
            {/*{ activeTab === "scheme" &&*/}
            {/*    <div style={{*/}
            {/*        display: "flex",*/}
            {/*        gap: "20px",*/}
            {/*        padding: "20px",*/}
            {/*        border: "2px solid #ccc",*/}
            {/*        overflowX: "auto"*/}
            {/*    }}>*/}
            {/*    /!*<SteampunkRail style={{*!/*/}
            {/*    /!*    flexShrink: 0,*!/*/}

            {/*    /!*    width: "239px",*!/*/}
            {/*    /!*    height: "20px",*!/*/}

            {/*    /!*    backgroundColor: "#FFFFFF",*!/*/}
            {/*    /!*    display: "flex",*!/*/}
            {/*    /!*    alignItems: "center",*!/*/}
            {/*    /!*    justifyContent: "center",*!/*/}
            {/*    /!*    borderRadius: "8px"*!/*/}
            {/*    /!*}}/>*!/*/}
            {/*    /!*<SteampunkRailR2 style={{*!/*/}
            {/*    /!*    flexShrink: 0,*!/*/}

            {/*    /!*    // width: "250px",*!/*/}
            {/*    /!*    // height: "50px",*!/*/}

            {/*    /!*    backgroundColor: "#FFFFFF",*!/*/}
            {/*    /!*    display: "flex",*!/*/}
            {/*    /!*    alignItems: "center",*!/*/}
            {/*    /!*    justifyContent: "center",*!/*/}
            {/*    /!*    borderRadius: "8px"*!/*/}
            {/*    /!*}}/>*!/*/}
            {/*        <CanvasPikoH0 style={{*/}
            {/*            flexShrink: 0,*/}

            {/*            width: "1500px",*/}
            {/*            height: "200px",*/}

            {/*            display: "flex",*/}
            {/*            alignItems: "center",*/}
            {/*            justifyContent: "center",*/}
            {/*        }}/>*/}
            {/*    </div>*/}
            {/*}*/}
            {items &&
                <>
                    { activeTab === "item_cards" && getGrid(items) }
                    {/*<Container fluid>*/}
                    {/*    <Row>*/}
                    {/*        <Col sm="3">*/}
                    {/*            <Accordion>*/}
                    {/*                { getAccordion(items) }*/}
                    {/*            </Accordion>*/}
                    {/*        </Col>*/}
                    {/*        <Col>*/}
                    {/*            { getGrid(items) }*/}
                    {/*        </Col>*/}
                    {/*    </Row>*/}
                    {/*</Container>*/}
                    {/*<TestCanvas width="400px" height="400px" theme="dark" unitLabel="x1000"*/}
                    {/*            startAngle={-45} endAngle={120} maxSpeed={10} unitMultiplier={1000}*/}
                    {/*            showSmallestLines={true} bigLabelK={20} middleLabelK={10} valueStep={0.1}/>*/}
                </>
            }

            {!items &&
                <Alert variant="danger">
                    Error load data
                </Alert>
            }
        </>
    );
}