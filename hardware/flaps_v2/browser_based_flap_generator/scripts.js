paper.install(window);

var flapGenerator

function updateFormFromActiveFlap(fg) {
    document.getElementById("flapScaleInput").value = fg.getActiveFlapScale();
    document.getElementById("flapThicknessInput").value = fg.getActiveFlapThickness();
    const [x, y] = fg.getActiveFlapPosition();
    document.getElementById("flapPositionXInput").value = x;
    document.getElementById("flapPositionYInput").value = y;
}

window.onload = async function () {
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
    canvas.addEventListener('wheel', function(event) {
        // Zoom in or out
        var zoomFactor = 1.1;
        var oldZoom = paper.view.zoom;
        if (event.deltaY < 0) {
            paper.view.zoom *= zoomFactor;
        } else {
            paper.view.zoom /= zoomFactor;
        }
        var newZoom = paper.view.zoom;
    
        // Calculate the zoom point
        var mousePosition = new paper.Point(event.offsetX, event.offsetY);
        var viewPosition = paper.view.viewToProject(mousePosition);
        var zoomDelta = newZoom / oldZoom;
    
        // Adjust the view center
        var centerAdjust = viewPosition.subtract(paper.view.center);
        var offset = centerAdjust.multiply(zoomDelta - 1);
        paper.view.center = paper.view.center.add(offset);
    
        // Prevent the page from scrolling
        event.preventDefault();
    });

    var tool = new paper.Tool();
    var panStartPoint;

    tool.onMouseDown = function(event) {
        // Store the starting point for panning
        panStartPoint = event.point;
    };

    tool.onMouseDrag = function(event) {
        // Calculate the delta
        var delta = event.downPoint.subtract(event.point);
        // Adjust the view's center
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

//     var rect = new Rectangle();
//     rect.size = new Size(100, 100);
//     var pathRect = new Path.Rectangle(rect);
//     pathRect.strokeColor = 'black';
//     pathRect.fillColor = 'red';
//     rect.center = new Point(100, 100);
//     pathRect2 = new Path.Rectangle(rect);
//     pathRect2.strokeColor = 'black';
//     pathRect2.fillColor = 'blue';

//     result = pathRect2.subtract(pathRect);
//     result.fillColor = 'green';

//     pathRect.remove();
//     pathRect2.remove();
// }
// var text;



// function drawPath(opentypePathSVG) {
//     text = paper.project.importSVG(opentypePathSVG);
//     text.strokeColor = 'black';
//     text.fillColor = 'red';

//     result = pathRect2.subtract(text);
//     result.fillColor = 'green';

//     text.remove();
//     pathRect2.remove();
// }


function save() {
    var svg = project.exportSVG({ asString: true });
    var blob = new Blob([svg], { type: 'image/svg+xml' });
    var url = URL.createObjectURL(blob);
    var link = document.createElement('a');
    link.href = url;
    link.download = 'canvas.svg';
    link.click();
    URL.revokeObjectURL(url);
}

// GERBOLYZE: gerbolyze convert -n altium flap.svg gerber.zip