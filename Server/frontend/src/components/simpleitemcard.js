import Card from "react-bootstrap/Card";
import React, {useState} from "react";

function getTypeName(item) {
    switch (item.type) {
        case "TRAIN": return "Локомотив";
        case "SWITCH": return "Стрелка";
        case "SEMAPHORE": return "Семафор";
    }

    return "Unknown";
}

export default function useSimpleItemCard(item) {
    console.log("Render: " + item.id);

    return (
        <Card style={{ width: '16rem' }}>
            <Card.Body>
                <Card.Title>{ item.id }</Card.Title>
                <Card.Subtitle className="mb-2 text-muted">{ getTypeName(item) }</Card.Subtitle>
                <Card.Text>{ item.data }</Card.Text>
                <Card.Link href="#">Card Link</Card.Link>
                <Card.Link href="#">Another Link</Card.Link>
            </Card.Body>
        </Card>
    );
}