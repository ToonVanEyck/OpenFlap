class Flap {
    template_svg;
    glyph_svg;
    index;
    location;
    gap_mm;

    glyph_scale;
    glyph_thickness;
    glyph_position_x;
    glyph_position_y;

    flap_glyph;
    flap_upper;
    flap_lower;
    flap_svg;

    constructor(parent, template_svg, glyph_svg, index, gap_mm) {
        this.parent = parent;
        this.template_svg = template_svg;
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

        let rect = new Rectangle();
        rect.size = this.flap_svg.bounds;
        rect.center = this.flap_svg.position;
        this.flap_highlight_svg = new Path.Rectangle(rect);
        this.flap_highlight_svg.scale(1.03, 1.03)
        this.flap_highlight_svg.strokeColor = 'red';
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

    __onMouseDown(event) {
        this.parent.__selectFlap(this.index);
    }

}

class FlapGenerator {
    paper;
    font;
    flap_template_svg_path_tag;
    gerbolyzer_template_node;
    gap_mm;
    fontsize;
    updateFormCallback;
    flaps = [];
    selected_flap = null;

    constructor(paper, canvas_id, updateFormCallback) {
        this.paper = paper;
        this.paper.setup(canvas_id);
        this.paper.view.zoom = 4;
        this.gap_mm = 0.5;
        // this.paper.view.center = new this.paper.Point(0, 0);
        this.paper.view.center = new this.paper.Point(this.paper.view.bounds.width / 2, this.paper.view.bounds.height / 2);
        this.__loadDefaultGerbolyzerTemplate();
        // Calculate font size based on the flap height
        let flap_temp = paper.project.importSVG(this.flap_template_svg_path_tag)
        this.fontsize = (flap_temp.bounds.height * 2 + this.gap_mm) * 1.06;
        flap_temp.remove();
        this.updateFormCallback = updateFormCallback;
    }

    __loadDefaultGerbolyzerTemplate() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'gerbolyzer_template.svg', false);
        xhr.send(null);
        const parser = new DOMParser();
        this.gerbolyzer_template_node = parser.parseFromString(xhr.responseText, 'text/xml');
        this.flap_template_svg_path_tag = this.gerbolyzer_template_node.getElementById("g-outline").getElementsByTagName('path')[0];
    }

    __setFont(font) {
        this.font = font;
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

    __combineFlapFrontAndBack(frontFlap, backFlap) {
        // Clone glyphs
        let front_glyph = frontFlap.flap_glyph.clone();
        let back_glyph = backFlap.flap_glyph.clone();
        // Reposition bottom glyph
        back_glyph.position = new paper.Point((frontFlap.flap_upper.bounds.width * 1.1) * (frontFlap.index + 1 / 2), frontFlap.flap_upper.bounds.height * 1.1 - frontFlap.gap_mm);
        back_glyph.position.x += backFlap.glyph_position_x;
        back_glyph.position.y -= backFlap.glyph_position_y;

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
        console.log(gerbolyzer_svg_node);
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
        for (let flap of this.flaps) {
            flap.destructor();
        }
        this.flaps = [];
        for (var i = 0; i < text.length; i++) {
            let glyph_svg = await this.font.getPath(text[i], 0, 0, this.fontsize).toSVG();
            this.flaps.push(new Flap(this, this.flap_template_svg_path_tag, glyph_svg, i, this.gap_mm));
        }
    }
}