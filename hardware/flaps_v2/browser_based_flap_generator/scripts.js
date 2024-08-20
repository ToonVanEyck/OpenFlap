var flapGenerator

function updateFormFromActiveFlap(fg) {
    document.getElementById("flapScaleInput").value = fg.getActiveFlapScale();
    document.getElementById("flapThicknessInput").value = fg.getActiveFlapThickness();
    const [x, y] = fg.getActiveFlapPosition();
    document.getElementById("flapPositionXInput").value = x;
    document.getElementById("flapPositionYInput").value = y;
}

window.onload = async function () {
    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);

    const myModal = new bootstrap.Modal(document.getElementById('popupWelcome'), {})
    myModal.show()

    flapGenerator = new FlapGenerator(paper, "canvas-1", updateFormFromActiveFlap);

    var flapGenerateButton = document.getElementById("flapGenerateButton");
    flapGenerateButton.addEventListener("click", function () {
        flapGenerator.__generateFromText(document.getElementById("flapTextInput").value.toUpperCase());
    });
    (async () => {
        let gf = await getGoogleFontUrl("https://fonts.googleapis.com/css2?family=Red+Hat+Mono:wght@600");
        let font = await loadFont(gf);
        flapGenerator.__setFont(font);
        flapGenerator.__generateFromText("@O.#€");
    })();
    document.getElementById("flapScaleInput").addEventListener("change", updateActiveFlapScale);
    document.getElementById("flapThicknessInput").addEventListener("change", updateActiveFlapThickness);
    document.getElementById("flapPositionXInput").addEventListener("change", updateActiveFlapPosition);
    document.getElementById("flapPositionYInput").addEventListener("change", updateActiveFlapPosition);
    document.getElementById("flapExportButton").addEventListener("click", exportSvgForGerbolyzation);

    // Add event listeners for zoom and pan
    var canvas = document.getElementById('canvas-1');
    canvas.addEventListener('wheel', function (event) {
        var zoomFactor = 1.1;
        var oldZoom = paper.view.zoom;
        if (event.deltaY < 0) {
            paper.view.zoom *= zoomFactor;
        } else {
            paper.view.zoom /= zoomFactor;
        }
        var newZoom = paper.view.zoom;
        var mousePosition = new paper.Point(event.offsetX, event.offsetY);
        var viewPosition = paper.view.viewToProject(mousePosition);
        var zoomDelta = newZoom / oldZoom;
        var centerAdjust = viewPosition.subtract(paper.view.center);
        var offset = centerAdjust.multiply(zoomDelta - 1);
        paper.view.center = paper.view.center.add(offset);
        event.preventDefault();
    });
    var tool = new paper.Tool();
    var panStartPoint;
    tool.onMouseDown = function (event) {
        panStartPoint = event.point;
    };
    tool.onMouseDrag = function (event) {
        var delta = event.downPoint.subtract(event.point);
        paper.view.center = paper.view.center.add(delta);
    };
}

function updateActiveFlapScale() {
    flapGenerator.setActiveFlapScale(document.getElementById("flapScaleInput").value);
}

function updateActiveFlapThickness() {
    flapGenerator.setActiveFlapThickness(document.getElementById("flapThicknessInput").value);
}

function updateActiveFlapPosition() {
    flapGenerator.setActiveFlapPosition(document.getElementById("flapPositionXInput").value, document.getElementById("flapPositionYInput").value);
}

function exportSvgForGerbolyzation() {
    flapGenerator.exportFlapSvgs();
}

function loadJson() {
    flapGenerator.loadJson();
}

function exportJson() {
    flapGenerator.exportJson();
}

function resizeCanvas() {
    var element = document.evaluate('/html/body/div/div[1]', document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;
    var size = element.getBoundingClientRect();
    var canvas = document.getElementById('canvas-1');
    canvas.width = size.width;
    canvas.height = size.height;
}