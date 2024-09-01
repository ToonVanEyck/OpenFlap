var flapGenerator
var fontInfo = {};

window.onload = async function () {
    // Get all important elements
    var projectModal = new bootstrap.Modal(document.getElementById('popupWelcome'), {})
    var canvas = document.getElementById('canvas-1');
    var projectLoadButton = document.getElementById("projectLoadButton")
    var projectLoadSelector = document.getElementById("projectLoadSelector")
    var projectSaveButton = document.getElementById("projectSaveButton")
    // var projectTemplate = document.getElementById("projectTemplate");
    var projectFont = document.getElementById("projectFont")
    var projectFontVariant = document.getElementById("projectFontVariant")
    var projectFontSize = document.getElementById("projectFontSize")
    var projectCharacterSet = document.getElementById("projectCharacterSet")
    var projectShowGuides = document.getElementById("projectShowGuides")
    var projectGuideBot = document.getElementById("projectGuideBot")
    var projectGuideTop = document.getElementById("projectGuideTop")
    var projectGlobalOffset = document.getElementById("projectGlobalOffset")
    var projectGenerateButton = document.getElementById("projectGenerateButton")
    var flapScale = document.getElementById("flapScale")
    var flapThickness = document.getElementById("flapThickness")
    var flapPositionX = document.getElementById("flapPositionX")
    var flapPositionY = document.getElementById("flapPositionY")

    // Resize the canvas
    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);

    // Load the fonts
    await populateFonts();

    // Show the modal
    projectModal.show()
    // Create the flap generator
    flapGenerator = new FlapGenerator(paper, "canvas-1", updateFlapPropertiesView);

    // Add event listeners: Load and Save Button
    projectLoadButton.addEventListener("click", function () { projectLoadSelector.click(); });
    projectLoadSelector.addEventListener("change", loadProjectJson);
    projectSaveButton.addEventListener("click", exportJson);

    // Add event listeners: Template and Flap Gap (UNUSED)
    // projectTemplate.addEventListener("click", function () { projectTemplateSelector.click(); });
    // projectTemplateSelector.addEventListener("change", loadProjectTemplate);
    // projectFlapGap.addEventListener("change", function () { flapGenerator.setFlapGap(projectFlapGap.value); });

    // Add event listeners: Font
    projectFont.addEventListener("change", updateFont);
    projectFontVariant.addEventListener("change", updateFont);
    projectFontSize.addEventListener("change", updateFont);

    // Add event listeners: Character Set
    projectCharacterSet.addEventListener("change", updatedCharacterSet);

    // Add event listeners: Guides
    projectShowGuides.addEventListener("change", updateGuides);
    projectGuideBot.addEventListener("change", updateGuides);
    projectGuideTop.addEventListener("change", updateGuides);

    // Add event listeners: Global Offset
    projectGlobalOffset.addEventListener("change", updateGlobalOffset);

    // Add event listeners: Generate Button
    projectGenerateButton.addEventListener("click", exportSvgForGerbolyzation);

    // Add event listeners: Flap Scale, Thickness, Position
    flapScale.addEventListener("change", updateActiveFlapScale);
    flapThickness.addEventListener("change", updateActiveFlapThickness);
    flapPositionX.addEventListener("change", updateActiveFlapPosition);
    flapPositionY.addEventListener("change", updateActiveFlapPosition);

    // Set default values: Font
    projectFont.value = "Red Hat Mono";
    await updateFont({ target: { id: 'projectFont' } });
    projectFontVariant.value = "600";
    await updateFont({ target: { id: 'projectFontVariant' } });
    projectFontSize.value = "72";
    await updateFont({ target: { id: 'projectFontSize' } });

    // Set default values: Character Set
    projectCharacterSet.value = "OPE!&@#€.";
    await updatedCharacterSet();

    // Set default values: Guides
    projectShowGuides.checked = true;
    projectGuideBot.value = 8;
    projectGuideTop.value = 10;
    updateGuides();

    // Set default values: Global Offset
    projectGlobalOffset.value = 0;
    updateGlobalOffset();

    // Add event listeners for zoom and pan
    setupCanvasNavigation(canvas);
}

function updateActiveFlapScale() {
    flapGenerator.setActiveFlapScale(Number(document.getElementById("flapScale").value));
}

function updateActiveFlapThickness() {
    flapGenerator.setActiveFlapThickness(Number(document.getElementById("flapThickness").value));
};

function updateActiveFlapPosition() {
    flapGenerator.setActiveFlapPosition(Number(document.getElementById("flapPositionX").value), Number(document.getElementById("flapPositionY").value));
}

async function updatedCharacterSet() {
    await flapGenerator.setCharacterSet(projectCharacterSet.value);
}

function resizeCanvas() {
    var element = document.evaluate('/html/body/div/div[1]', document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;
    var size = element.getBoundingClientRect();
    var canvas = document.getElementById('canvas-1');
    canvas.width = size.width;
    canvas.height = size.height;

    try {
        flapGenerator.resizeCanvas(size.width, size.height);
    } catch (e) { }
}

function loadGoogleFont(fontName) {
    const link = document.createElement('link');
    link.href = `https://fonts.googleapis.com/css2?family=${fontName.replace(/ /g, '+')}&display=swap`;
    link.rel = 'stylesheet';
    document.head.appendChild(link);
}

async function populateFonts() {
    const API_KEY = 'AIzaSyBbedhccDcI_PS-owwtB9tF_cVLymvV7i0';
    const API_URL = `https://www.googleapis.com/webfonts/v1/webfonts?key=${API_KEY}`;
    const dropdown = document.getElementById('projectFont');

    try {
        const response = await fetch(API_URL);
        const data = await response.json();
        const fonts = data.items;

        fonts.forEach(font => {
            if (font.category !== 'monospace') return;
            fontInfo[font.family] = font;
            loadGoogleFont(font.family);
            const option = document.createElement('option');
            option.value = font.family;
            option.textContent = font.family;
            option.style.fontFamily = `${font.family}, ui-monospace`;
            dropdown.appendChild(option);
        });
    } catch (error) {
        console.error('Error fetching Google Fonts:', error);
    }
}

function populateFontVariants(font) {
    const dropdown = document.getElementById('projectFontVariant');
    dropdown.innerHTML = '';
    font.variants.forEach(variant => {
        const option = document.createElement('option');
        let variantTxt = variant;
        variantTxt = (variantTxt === 'italic') ? '400 italic' : variantTxt;
        variantTxt = variantTxt.replace('regular', '400');
        variantTxt = variantTxt.replace('italic', ' italic');
        option.value = variant;
        option.textContent = variantTxt;

        option.style.fontFamily = `${font.family}, ui-monospace`;
        const weight = variant.match(/\d+/);
        option.style.fontWeight = weight;
        option.style.fontStyle = variant.includes('italic') ? 'italic' : 'normal';

        dropdown.appendChild(option);
    });
    dropdown.value = "regular";
}

async function updateFont(event) {
    const projectFont = document.getElementById('projectFont');
    const projectFontVariant = document.getElementById('projectFontVariant');
    const projectFontSize = document.getElementById('projectFontSize');
    const fontFamily = projectFont.value;
    const fontObject = fontInfo[fontFamily];
    if (event.target.id === 'projectFont') {
        populateFontVariants(fontObject);
    }
    let variant = projectFontVariant.value;
    const size = Number(projectFontSize.value);

    let weight = variant.match(/\d+/);
    weight = Number(weight ? weight[0] : 400);
    const style = variant.includes('italic') ? 'italic' : 'normal';

    projectFont.style.fontFamily = `${fontFamily}, ui-monospace`;
    projectFont.style.fontWeight = weight
    projectFont.style.fontStyle = style

    projectFontVariant.style.fontFamily = `${fontFamily}, ui-monospace`;
    projectFontVariant.style.fontWeight = weight
    projectFontVariant.style.fontStyle = style

    const buffer = fetch(fontObject.files[variant]).then(res => res.arrayBuffer());
    const font = opentype.parse(await buffer);

    flapGenerator.setFont(fontFamily, size, weight, style, font);
}

function updateGuides() {
    flapGenerator.setGuideLines(projectShowGuides.checked, Number(projectGuideBot.value), Number(projectGuideTop.value));
}

function updateGlobalOffset() {
    flapGenerator.setGlobalOffset(Number(projectGlobalOffset.value));
}

function exportSvgForGerbolyzation() {
    flapGenerator.exportFlapSvgs();
}

function exportJson() {
    let data = {
        gerbolyzer_template_b64: btoa(new XMLSerializer().serializeToString(flapGenerator.gerbolyzer_template_node)),
        gap_mm: flapGenerator.gap_mm,
        font: {
            family: flapGenerator.fontFamily,
            weight: flapGenerator.fontWeight,
            size: flapGenerator.fontSize,
            style: flapGenerator.fontStyle,
        },
        guides: {
            show: flapGenerator.show_guides,
            top: flapGenerator.top_guide_offset,
            bot: flapGenerator.bot_guide_offset,
        },
        globalOffset: flapGenerator.flaps[0].getGlobalOffset(),
        flaps: [],
    };
    for (let flap of flapGenerator.flaps) {
        let [x, y] = flap.getPosition();
        data.flaps.push({
            character: flap.getCharacter(),
            scale: flap.getScale(),
            thickness: flap.getThickness(),
            position: { x: x, y: y },
        });
    }
    const json = JSON.stringify(data, null, 4);
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'flapGeneratorProject.json';
    link.click();
}

async function loadJson(json) {
    flapGenerator.__clearFlaps();
    let data = JSON.parse(json);
    flapGenerator.gerbolyzer_template_node = new DOMParser().parseFromString(atob(data.gerbolyzer_template_b64), 'text/xml');
    flapGenerator.flap_template_svg_path_tag = flapGenerator.gerbolyzer_template_node.getElementById("g-outline").getElementsByTagName('path')[0];
    flapGenerator.setFlapGap(flapGenerator.gap_mm);

    projectFont.value = data.font.family;
    await updateFont({ target: { id: 'projectFont' } });
    projectFontVariant.value = (data.font.weight == 400) ? '' : data.font.weight + (data.font.style === 'italic' ? ' italic' : '');
    await updateFont({ target: { id: 'projectFontVariant' } });
    projectFontSize.value = data.font.size;
    await updateFont({ target: { id: 'projectFontSize' } });

    let characterSet = "";
    for (let flap of data.flaps) {
        characterSet += flap.character;
    }
    projectCharacterSet.value = characterSet;
    await updatedCharacterSet();

    projectShowGuides.checked = data.guides.show;
    projectGuideBot.value = data.guides.bot;
    projectGuideTop.value = data.guides.top;
    updateGuides();

    for (i = 0; i < data.flaps.length; i++) {
        flapGenerator.flaps[i].setScale(data.flaps[i].scale);
        flapGenerator.flaps[i].setThickness(data.flaps[i].thickness);
        flapGenerator.flaps[i].setPosition(data.flaps[i].position.x, data.flaps[i].position.y);
        flapGenerator.flaps[i].setGlobalOffset(data.globalOffset);
    }
}

function updateFlapPropertiesView(flap) {
    flapScale.value = flap.getActiveFlapScale();
    flapThickness.value = flap.getActiveFlapThickness();
    const [x, y] = flap.getActiveFlapPosition();
    flapPositionX.value = x;
    flapPositionY.value = y;
}

function loadProjectJson(e) {
    const reader = new FileReader();
    reader.onload = function (e) {
        loadJson(e.target.result);
    };
    reader.readAsText(e.target.files[0]);
}

function loadProjectTemplate(e) {
    var reader = new FileReader();
    reader.onload = function (e) {
        flapGenerator.setGerbolyzerTemplate(e.target.result);
    };
    reader.readAsText(e.target.files[0])
}

function setupCanvasNavigation(canvas) {
    canvas.style.cursor = 'grab';
    canvas.addEventListener('wheel', function (event) {
        var zoomFactor = 1.1;
        var oldZoom = paper.view.zoom;
        if (event.deltaY !== 0) {
            if (event.deltaY < 0) {
                paper.view.zoom *= zoomFactor;
            } else {
                paper.view.zoom /= zoomFactor;
            }
        } else if (event.deltaX !== 0) {
            paper.view.scrollBy(new paper.Point(event.deltaX / 10, 0));
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
        canvas.style.cursor = 'grabbing';
    };
    tool.onMouseUp = function (event) {
        canvas.style.cursor = 'grab';
    }
    tool.onMouseDrag = function (event) {
        var delta = event.downPoint.subtract(event.point);
        paper.view.center = paper.view.center.add(delta);
    };
}