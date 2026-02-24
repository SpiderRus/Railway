var tempFormatter = undefined;

var tempChart = undefined;

var tempTimer;

async function updateTempData(itemId) {
    while (tempChart !== undefined) {
        let now = Date.now();

        let response = await (await fetch("/api/temperature-chart/" + itemId)).json();

        if ((tempChart !== undefined)) {
            tempChart.update({
                series: [{
                    data: response.map((point) => point.y === null ? {
                        x: point.x,
                        y: undefined
                    } : point)
                }]
            });

            let timeout = 40 - (Date.now() - now);

            if (timeout > 0) {
                tempTimer = setTimeout(() => updateTempData(itemId), timeout);
                break;
            }
        }
    }
}

function destroyTemp() {
    if (tempChart !== undefined) {
        clearTimeout(tempTimer);
        tempChart.detach();
        tempChart = undefined;
    }
}

function initTemp(itemId, chartClass) {
    if (tempFormatter === undefined)
        tempFormatter = new Intl.DateTimeFormat(undefined, { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    destroyTemp();

    tempChart = new Chartist.Line(chartClass, {
        labels: [],
        series: []
    }, {
        // lineSmooth: Chartist.Interpolation.simple({
        //     divisor: 2
        // }),
        lineSmooth: Chartist.Interpolation.none(),
        showPoint: false,
        fullWidth: true,
        chartPadding: {
            right: 40
        },
        axisX: {
            type: Chartist.FixedScaleAxis,
            divisor: 8,
            labelInterpolationFnc: (value) => tempFormatter.format(new Date(value))
        },
        axisY: {
            type: Chartist.AutoScaleAxis,
            onlyInteger: true,
            // type: Chartist.FixedScaleAxis,
            // ticks: [0, 50, 75, 100],
            low: 0,
            high: 80
        }
    });

    updateTempData(itemId);
}
