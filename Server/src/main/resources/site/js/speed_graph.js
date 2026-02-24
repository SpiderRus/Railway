var speedFormatter = undefined;

var speedChart = undefined;

var speedTimer;

async function updateSpeedData(itemId) {
    while (speedChart !== undefined) {
        let now = Date.now();

        let response = await (await fetch("/api/speed-chart/" + itemId)).json();

        if ((speedChart !== undefined)) {
            speedChart.update({
                series: [{
                    data: response.map((point) => point.y === null ? {
                        x: point.x,
                        y: undefined
                    } : point)
                }]
            });

            let timeout = 40 - (Date.now() - now);

            if (timeout > 0) {
                speedTimer = setTimeout(() => updateSpeedData(itemId), timeout);
                break;
            }
        }
    }
}

function destroySpeed() {
    if (speedChart !== undefined) {
        clearTimeout(speedTimer);
        speedChart.detach();
        speedChart = undefined;
    }
}

function initSpeed(itemId, chartClass) {
    if (speedFormatter === undefined)
        speedFormatter = new Intl.DateTimeFormat(undefined, { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    destroySpeed();

    speedChart = new Chartist.Line(chartClass, {
        labels: [],
        series: []
    }, {
        // lineSmooth: Chartist.Interpolation.simple({
        //     divisor: 2
        // }),
        lineSmooth: Chartist.Interpolation.none(),
        showPoint: true,
        fullWidth: true,
        chartPadding: {
            right: 40
        },
        axisX: {
            type: Chartist.FixedScaleAxis,
            divisor: 8,
            labelInterpolationFnc: (value) => speedFormatter.format(new Date(value))
        },
        axisY: {
            type: Chartist.AutoScaleAxis,
            onlyInteger: true,
            // type: Chartist.FixedScaleAxis,
            // ticks: [0, 50, 75, 100],
            low: 0,
            high: 100
        }
    });

    updateSpeedData(itemId);
}
