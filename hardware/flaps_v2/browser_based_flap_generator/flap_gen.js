class Flap {
    template_svg;
    character;
    glyph_svg;
    index;
    location;
    gap_mm;

    glyph_scale;
    glyph_thickness;
    glyph_position_x;
    glyph_position_y;
    glyph_global_offset;

    flap_glyph;
    flap_upper;
    flap_lower;
    flap_svg;

    constructor(parent, character, template_svg, glyph_svg, index, gap_mm) {
        this.parent = parent;
        this.template_svg = template_svg;
        this.character = character
        this.glyph_svg = glyph_svg;
        this.index = index;
        this.gap_mm = gap_mm;

        this.flap_svg = new paper.Group();
        this.flap_highlight_svg = new paper.Group();

        // Draw upper half of the flap
        this.flap_upper = paper.project.importSVG(this.template_svg);
        this.flap_upper.fillColor = 'black';
        this.flap_upper.position = new paper.Point((this.flap_upper.bounds.width * 1.1) * (index + 1 / 2), this.flap_upper.bounds.height * 1.1 / 2);
        this.flap_upper.parent = this.flap_svg;
        // Draw lower half of the flap
        this.flap_lower = this.flap_upper.clone();
        this.flap_lower.scale(1, -1);
        this.flap_lower.position.y += this.flap_upper.bounds.height + this.gap_mm;
        // Draw the glyph
        this.flap_glyph = paper.project.importSVG(this.glyph_svg);
        this.flap_glyph.fillColor = 'white';
        this.flap_glyph.strokeColor = 'white';
        this.flap_glyph.position = new paper.Point((this.flap_upper.bounds.width * 1.1) * (index + 1 / 2), this.flap_upper.bounds.height * 1.1 - this.gap_mm);
        this.flap_glyph.parent = this.flap_svg;
        this.flap_glyph.strokeWidth = 0;

        this.glyph_scale = 1;
        this.glyph_thickness = 0;
        this.glyph_position_x = 0;
        this.glyph_position_y = 0;
        this.glyph_global_offset = 0;

        let rect = new paper.Rectangle();
        rect.size = this.flap_svg.bounds;
        rect.center = this.flap_svg.position;
        this.flap_highlight_svg = new paper.Path.Rectangle(rect);
        this.flap_highlight_svg.scale(1.03, 1.03)
        this.flap_highlight_svg.strokeColor = getComputedStyle(document.body).getPropertyValue('--bs-primary-text-emphasis');
        this.flap_highlight_svg.strokeWidth = 0;

        this.flap_svg.onMouseDown = this.__onMouseDown.bind(this);
    }

    destructor() {
        // Clean up any resources or event listeners here
        this.flap_svg.remove();
        this.flap_highlight_svg.remove();
        this.flap_upper.remove();
        this.flap_lower.remove();
        this.flap_glyph.remove();
    }

    select() {
        this.flap_highlight_svg.strokeWidth = 0.5;
    }

    deselect() {
        this.flap_highlight_svg.strokeWidth = 0;
    }

    setScale(scale) {
        if (scale < 0.001) {
            scale = 0.001;
        }
        this.flap_glyph.scale(scale / this.glyph_scale, scale / this.glyph_scale);
        this.glyph_scale = scale;
    }

    getScale() {
        return this.glyph_scale;
    }

    setThickness(thickness) {
        this.flap_glyph.strokeWidth = thickness;
        this.glyph_thickness = thickness;
    }

    getThickness() {
        return this.glyph_thickness;
    }

    setPosition(x, y) {
        this.flap_glyph.position.x += x - this.glyph_position_x;
        this.flap_glyph.position.y -= y - this.glyph_position_y;
        this.glyph_position_x = Number(x);
        this.glyph_position_y = Number(y);
    }

    getPosition() {
        return [this.glyph_position_x, this.glyph_position_y];
        return [this.flap_glyph.position.x, this.flap_glyph.position.y];
    }

    setGlobalOffset(y) {
        this.flap_glyph.position.y -= y - this.glyph_global_offset;
        this.glyph_global_offset = Number(y);
    }

    getGlobalOffset() {
        return this.glyph_global_offset;
    }

    getCharacter() {
        return this.character;
    }

    __onMouseDown(event) {
        this.parent.__selectFlap(this.index);
    }

}

class FlapGenerator {
    paper;
    flap_template_svg_path_tag;
    gerbolyzer_template_node;
    gap_mm;
    fontFamily;
    fontWeight;
    fontSize;
    fontStyle;
    updateFormCallback;
    flaps = [];
    selected_flap = null;

    bot_guide;
    top_guide;
    top_guide_offset;
    bot_guide_offset;
    show_guides;

    constructor(paper, canvas_id, updateFormCallback) {
        this.paper = paper;
        this.paper.setup(canvas_id);
        this.gap_mm = 0.5;
        this.bot_guide_offset = 1;
        this.top_guide_offset = 1;
        this.show_guides = false;
        this.__loadDefaultGerbolyzerTemplate();
        let flap_temp = paper.project.importSVG(this.flap_template_svg_path_tag)
        flap_temp.remove();
        this.updateFormCallback = updateFormCallback;
    }

    __loadDefaultGerbolyzerTemplate() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'gerbolyzer_template.svg', false);
        xhr.send(null);
        this.setGerbolyzerTemplate(xhr.responseText);
    }

    __selectFlap(index) {
        if (this.selected_flap) {
            this.selected_flap.deselect();
        }
        this.selected_flap = this.flaps[index];
        this.selected_flap.select();
        this.updateFormCallback(this);
    }

    setActiveFlapScale(scale) {
        if (this.selected_flap) {
            this.selected_flap.setScale(scale);
        }
    }

    getActiveFlapScale() {
        if (this.selected_flap) {
            return this.selected_flap.getScale();
        }
    }

    setActiveFlapThickness(thickness) {
        if (this.selected_flap) {
            this.selected_flap.setThickness(thickness);
        }
    }

    getActiveFlapThickness() {
        if (this.selected_flap) {
            return this.selected_flap.getThickness();
        }
    }

    setActiveFlapPosition(x, y) {
        if (this.selected_flap) {
            this.selected_flap.setPosition(x, y);
        }
    }

    getActiveFlapPosition() {
        if (this.selected_flap) {
            return this.selected_flap.getPosition();
        }
    }

    setFlapGap(gap_mm) {
        this.__clearFlaps();
        this.gap_mm = gap_mm;
    }

    getCharacterSet() {
        let characterSet = "";
        for (let flap of this.flaps) {
            characterSet += flap.getCharacter();
        }
        return characterSet;
    }

    async setCharacterSet(characterSet) {
        let selectedIndex = this.flaps.findIndex(flap => flap === this.selected_flap);
        selectedIndex = selectedIndex == -1 ? 0 : selectedIndex;
        const globalOffset = (this.flaps.length) ? this.flaps[0].getGlobalOffset() : 0;
        let propertiesMap = {}
        for (let flap of this.flaps) {
            propertiesMap[flap.getCharacter()] = {
                scale: flap.getScale(),
                thickness: flap.getThickness(),
                position: flap.getPosition(),
            };
        }
        await this.__generateFromText(characterSet);
        for (let flap of this.flaps) {
            if (flap.getCharacter() in propertiesMap) {
                flap.setScale(propertiesMap[flap.getCharacter()].scale);
                flap.setThickness(propertiesMap[flap.getCharacter()].thickness);
                flap.setPosition(propertiesMap[flap.getCharacter()].position[0], propertiesMap[flap.getCharacter()].position[1]);
                flap.setGlobalOffset(globalOffset);
            }
        }
        const totalFlaps = this.flaps.length;
        if (totalFlaps > 0) {
            const clippedIndex = Math.max(0, Math.min(selectedIndex, totalFlaps - 1));
            this.__selectFlap(clippedIndex);
        }
    }

    setFont(fontFamily, fontSize, fontWeight, fontStyle, ttf) {
        this.fontFamily = fontFamily;
        this.fontSize = fontSize;
        this.fontWeight = fontWeight;
        this.fontStyle = fontStyle;
        this.font = ttf;
        this.setCharacterSet(this.getCharacterSet());
    }

    getFont() {
        return [this.fontFamily, this.fontSize, this.fontWeight, this.fontStyle];
    }

    __combineFlapFrontAndBack(frontFlap, backFlap) {
        // Clone glyphs
        let front_glyph = PaperOffset.offset(frontFlap.flap_glyph, frontFlap.getThickness() / 2);
        let back_glyph = PaperOffset.offset(backFlap.flap_glyph, backFlap.getThickness() / 2);
        // Reposition bottom glyph
        back_glyph.position = new paper.Point((frontFlap.flap_upper.bounds.width * 1.1) * (frontFlap.index + 1 / 2), frontFlap.flap_upper.bounds.height * 1.1 - frontFlap.gap_mm);
        back_glyph.position.x += backFlap.glyph_position_x;
        back_glyph.position.y -= backFlap.glyph_position_y;

        frontFlap.flap_glyph.remove();

        // Intersect glyphs
        let front_glyph_intersect = frontFlap.flap_upper.intersect(front_glyph);
        let back_glyph_intersect = frontFlap.flap_lower.intersect(back_glyph);
        back_glyph_intersect.scale(1, -1);
        back_glyph_intersect.position.y -= back_glyph_intersect.bounds.height + frontFlap.gap_mm;
        front_glyph.remove();
        back_glyph.remove();
        front_glyph.remove();
        back_glyph.remove();

        // Reposition glyphs
        front_glyph_intersect.position.x -= frontFlap.flap_upper.bounds.x
        back_glyph_intersect.position.y -= frontFlap.flap_upper.bounds.y
        back_glyph_intersect.position.x -= frontFlap.flap_upper.bounds.x
        front_glyph_intersect.position.y -= frontFlap.flap_upper.bounds.y

        // Add glyphs to output svg
        let gerbolyzer_svg_node = this.gerbolyzer_template_node.cloneNode(true);
        gerbolyzer_svg_node.getElementById('g-top-silk').appendChild(front_glyph_intersect.exportSVG());
        gerbolyzer_svg_node.getElementById('g-bottom-silk').appendChild(back_glyph_intersect.exportSVG());

        // Remove intersected glyphs
        front_glyph_intersect.remove();
        back_glyph_intersect.remove();

        return gerbolyzer_svg_node;
    }

    exportFlapSvgs() {
        const zip = new JSZip();
        const promises = [];

        for (let i = 0; i < this.flaps.length; i++) {
            const currentFlap = this.flaps[i];
            const nextFlap = this.flaps[(i + 1) % this.flaps.length];
            const svgNode = this.__combineFlapFrontAndBack(currentFlap, nextFlap);
            const svgData = new XMLSerializer().serializeToString(svgNode);
            const blob = new Blob([svgData], { type: 'image/svg+xml' });
            const fileName = `flap_${currentFlap.index}.svg`;

            zip.file(fileName, blob);
        }

        Promise.all(promises).then(() => {
            zip.generateAsync({ type: 'blob' }).then((content) => {
                saveAs(content, 'flaps.zip');
            });
        });
    }

    async __generateFromText(text) {
        if (this.font === undefined) {
            return;
        }
        this.__clearFlaps();
        this.flaps = [];
        for (var i = 0; i < text.length; i++) {
            let glyph_svg = await this.font.getPath(text[i], 0, 0, this.fontSize).toSVG();
            this.flaps.push(new Flap(this, text[i], this.flap_template_svg_path_tag, glyph_svg, i, this.gap_mm));
        }
        try {
            this.setGuideLines(this.show_guides, this.bot_guide_offset, this.top_guide_offset);
            this.top_guide.bringToFront();
            this.bot_guide.bringToFront();
        } catch (e) { }
    }

    setGerbolyzerTemplate(template_svg) {
        this.__clearFlaps();
        const parser = new DOMParser();
        this.gerbolyzer_template_node = parser.parseFromString(template_svg, 'text/xml');
        this.flap_template_svg_path_tag = this.gerbolyzer_template_node.getElementById("g-outline").getElementsByTagName('path')[0];
    }

    __clearFlaps() {
        this.selected_flap = null;
        for (let flap of this.flaps) {
            flap.destructor();
        }
        this.flaps = [];
    }

    setGuideLines(visible, bottomOffset, topOffset) {
        this.show_guides = visible;
        this.bot_guide_offset = Number(bottomOffset);
        this.top_guide_offset = Number(topOffset);

        try {
            this.bot_guide.remove();
            this.top_guide.remove();
        } catch (e) { }

        if (!this.show_guides) {
            return;
        }

        let x1 = this.flaps[0].flap_lower.bounds.x
        let x2 = this.flaps[this.flaps.length - 1].flap_lower.bounds.x + this.flaps[this.flaps.length - 1].flap_lower.bounds.width
        let y_b = this.flaps[0].flap_lower.bounds.y + this.flaps[0].flap_lower.bounds.height - this.bot_guide_offset
        let y_t = this.flaps[0].flap_upper.bounds.y + this.top_guide_offset

        this.bot_guide = new paper.Path();
        this.bot_guide.strokeColor = new paper.Color(0, 255, 0, 0.5);
        this.bot_guide.strokeWidth = 0.1;
        this.bot_guide.add(new paper.Point(x1, y_b));
        this.bot_guide.add(new paper.Point(x2, y_b));

        this.top_guide = new paper.Path();
        this.top_guide.strokeColor = new paper.Color(0, 255, 0, 0.5);
        this.top_guide.strokeWidth = 0.1;
        this.top_guide.add(new paper.Point(x1, y_t));
        this.top_guide.add(new paper.Point(x2, y_t));
    }

    getGuideLines() {
        return [this.show_guides, this.bot_guide_offset, this.top_guide_offset];
    }

    setGlobalOffset(y) {
        for (let flap of this.flaps) {
            flap.setGlobalOffset(y);
        }
    }

    getGlobalOffset() {
        return this.flaps[0].getGlobalOffset();
    }

    centerView() {
        const contentBounds = paper.project.activeLayer.bounds;
        const viewBounds = paper.view.bounds;
        const currentZoom = paper.view.zoom;
        const scaleX = viewBounds.width / contentBounds.width;
        const scaleY = viewBounds.height / contentBounds.height;
        const scale = Math.min(scaleX, scaleY) * 0.98;
        const newZoom = currentZoom * scale;
        if (isFinite(newZoom)) {
            this.paper.view.center = contentBounds.center;
            this.paper.view.zoom = newZoom;
        }
    }

    resizeCanvas(width, height) {
        paper.view.viewSize.width = width;
        paper.view.viewSize.height = height;
        centerView();
    }
} 