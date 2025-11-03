
const moduleEndpoint = "/api/module"
// const moduleEndpoint = "http://192.168.0.45:80/api/module" // enable this line for local development

const controllerFirmwareEndpoint = "/api/controller/firmware.bin";
const moduleFirmwareEndpoint = "/api/module/firmware.bin";

var moduleObjects = [];
var dimensions = { width: 0, height: 0 };

async function moduleGetAll() {
    const response = await fetch(moduleEndpoint, {
        method: "GET",
        headers: { "Content-Type": "application/json" }
    });
    console.log(response);
    return await response.json();
}

const elNew = (tag, prop) => Object.assign(document.createElement(tag), prop);

var activeDisplayIndex = () => [...document.querySelectorAll(".displayCharacter")].indexOf(document.activeElement)
var displayCharacterAt = (index) => [...document.querySelectorAll(".displayCharacter")][index]
var activeAnyIndex = () => activeDisplayIndex();
var activeCharacterSetEntry = () => document.activeElement.id.split('_').map(Number).slice(1, 3);
var characterSetEntryAt = (m_index, c_index) => document.getElementById("characterSetMember_" + m_index + "_" + c_index);

function getCheckboxFromSetting(setting) {
    let node = setting.parentNode;
    let checkbox = node.querySelector('input[type="checkbox"]');
    while (!checkbox && node.previousElementSibling) {
        node = node.previousElementSibling;
        checkbox = node.querySelector('input[type="checkbox"]');
    }
    return checkbox;
}

function moduleSettingsFromModule(template, module) {
    let module_settings = template.cloneNode(true);
    const i = module.module;
    module_settings.id = "module_settings_" + i;
    module_settings.style.display = "";
    module_settings.querySelectorAll("[id]").forEach(el => {
        el.id = el.id.replace("#", i);
    });
    module_settings.children[0].innerText = "Module " + i;

    const updateSettingsField = (input, cnt) => {
        const checkbox = getCheckboxFromSetting(input);
        if (checkbox) {
            checkbox.checked = true;
        }

        if (cnt) {
            cnt.innerText = input.value.length;
        }
    };

    let info_column_end_element = module_settings.querySelector("#module_info_column_end_" + i)
    let info_type_element = module_settings.querySelector("#module_info_type_" + i)
    let version_element = module_settings.querySelector("#module_version_" + i)
    let encoder_offset_element = module_settings.querySelector("#module_encoder_offset_" + i)
    let character_element = module_settings.querySelector("#module_character_" + i)
    let character_set_element = module_settings.querySelector("#module_character_set_" + i)
    let character_set_cnt_element = module_settings.querySelector("#module_character_set_cnt_" + i)
    let character_set_max_element = module_settings.querySelector("#module_character_set_max_" + i)
    let motion_minimum_speed_element = module_settings.querySelector("#module_motion_minimum_speed_" + i)
    let motion_maximum_speed_element = module_settings.querySelector("#module_motion_maximum_speed_" + i)
    let motion_ramp_start_element = module_settings.querySelector("#module_motion_ramp_start_" + i)
    let motion_ramp_stop_element = module_settings.querySelector("#module_motion_ramp_stop_" + i)
    let ir_threshold_upper_element = module_settings.querySelector("#module_ir_threshold_upper_" + i)
    let ir_threshold_lower_element = module_settings.querySelector("#module_ir_threshold_lower_" + i)
    let minimum_rotation_element = module_settings.querySelector("#module_minimum_rotation_" + i)
    let color_foreground_element = module_settings.querySelector("#module_color_foreground_" + i)
    let color_background_element = module_settings.querySelector("#module_color_background_" + i)

    info_column_end_element.innerText = module.module_info.column_end ? "True" : "False";
    info_type_element.innerText = module.module_info.type;
    version_element.innerText = module.firmware_version.description.match(/^v\d+\.\d+\.\d+/)[0];
    encoder_offset_element.value = module.offset;
    character_element.value = module.character;
    character_set_element.innerText = module.character_set.join("").replace(/ /g, '\u00A0');
    character_set_element.maxLength = module.character_set.length;
    character_set_cnt_element.innerText = module.character_set.length;
    character_set_max_element.innerText = module.character_set.length;
    motion_minimum_speed_element.value = module.motion.speed_min;
    motion_maximum_speed_element.value = module.motion.speed_max;
    motion_ramp_start_element.value = module.motion.ramp_start;
    motion_ramp_stop_element.value = module.motion.ramp_stop;
    ir_threshold_upper_element.value = module.ir_threshold.upper;
    ir_threshold_lower_element.value = module.ir_threshold.lower;
    minimum_rotation_element.value = module.minimum_rotation;
    color_foreground_element.value = module.color.foreground;
    color_background_element.value = module.color.background;

    // Add onchange event listeners
    encoder_offset_element.onchange = () => updateSettingsField(encoder_offset_element);
    character_element.onchange = () => updateSettingsField(character_element);
    character_set_element.onchange = () => updateSettingsField(character_set_element, character_set_cnt_element);
    motion_minimum_speed_element.onchange = () => updateSettingsField(motion_minimum_speed_element);
    motion_maximum_speed_element.onchange = () => updateSettingsField(motion_maximum_speed_element);
    motion_ramp_start_element.onchange = () => updateSettingsField(motion_ramp_start_element);
    motion_ramp_stop_element.onchange = () => updateSettingsField(motion_ramp_stop_element);
    ir_threshold_upper_element.onchange = () => updateSettingsField(ir_threshold_upper_element);
    ir_threshold_lower_element.onchange = () => updateSettingsField(ir_threshold_lower_element);
    minimum_rotation_element.onchange = () => updateSettingsField(minimum_rotation_element);
    color_foreground_element.onchange = () => updateSettingsField(color_foreground_element);
    color_background_element.onchange = () => updateSettingsField(color_background_element);
    return module_settings;
}

function moduleSettingsToModule(i) {
    module_settings = document.getElementById("module_settings_" + i);
    let json = {
        module: i
    };

    let encoder_offset_element = module_settings.querySelector("#module_encoder_offset_" + i);
    if (getCheckboxFromSetting(encoder_offset_element).checked) {
        json.offset = parseInt(encoder_offset_element.value);
    }
    let character_element = module_settings.querySelector("#module_character_" + i);
    if (getCheckboxFromSetting(character_element).checked) {
        json.character = character_element.value;
    }
    let character_set_element = module_settings.querySelector("#module_character_set_" + i);
    if (character_set_element.checked) {
        json.character_set = character_set_element.innerText.split("");
    }
    let motion_minimum_speed_element = module_settings.querySelector("#module_motion_minimum_speed_" + i)
    let motion_maximum_speed_element = module_settings.querySelector("#module_motion_maximum_speed_" + i)
    let motion_ramp_start_element = module_settings.querySelector("#module_motion_ramp_start_" + i)
    let motion_ramp_stop_element = module_settings.querySelector("#module_motion_ramp_stop_" + i)
    if (getCheckboxFromSetting(motion_minimum_speed_element).checked) {
        json.motion = json.motion || {};
        json.motion.speed_min = parseInt(motion_minimum_speed_element.value);
        json.motion.speed_max = parseInt(motion_maximum_speed_element.value);
        json.motion.ramp_start = parseInt(motion_ramp_start_element.value);
        json.motion.ramp_stop = parseInt(motion_ramp_stop_element.value);
    }
    let ir_threshold_upper_element = module_settings.querySelector("#module_ir_threshold_upper_" + i)
    let ir_threshold_lower_element = module_settings.querySelector("#module_ir_threshold_lower_" + i)
    if (getCheckboxFromSetting(ir_threshold_upper_element).checked) {
        json.ir_threshold = json.ir_threshold || {};
        json.ir_threshold.upper = parseInt(ir_threshold_upper_element.value);
        json.ir_threshold.lower = parseInt(ir_threshold_lower_element.value);
    }
    let minimum_rotation_element = module_settings.querySelector("#module_minimum_rotation_" + i)
    if (getCheckboxFromSetting(minimum_rotation_element).checked) {
        json.minimum_rotation = parseInt(minimum_rotation_element.value);
    }
    let color_foreground_element = module_settings.querySelector("#module_color_foreground_" + i)
    let color_background_element = module_settings.querySelector("#module_color_background_" + i)
    if (getCheckboxFromSetting(color_foreground_element).checked) {
        json.color = json.color || {};
        json.color.foreground = color_foreground_element.value;
        json.color.background = color_background_element.value;
    }

    return json;
}

function createModuleTable() {
    let table = document.getElementById("moduleTable");
    let module_settings_template = document.getElementById("module_settings_#")
    module_settings_template.style.display = "none";
    while (module_settings_template.nextSibling) {
        module_settings_template.parentNode.removeChild(module_settings_template.nextSibling);
    }
    for (let i = 0; i < moduleObjects.length; i++) {
        const module = moduleObjects[i];
        let module_settings = moduleSettingsFromModule(module_settings_template, module);
        module_settings_template.parentElement.appendChild(module_settings);
    }
}

function calculateDisplayDimensions() {
    var width = 0;
    moduleObjects.forEach(module => width += (module.module_info.column_end));
    var height = moduleObjects.length / width;
    if (!Number.isInteger(height)) {
        console.log("The display is not rectangular!");
        return
    }
    dimensions.height = height;
    dimensions.width = width
    let root = document.querySelector(':root');
    root.style.setProperty('--dispWidth', dimensions.width);
    root.style.setProperty('--dispHeight', dimensions.height);
}

function createDisplay() {
    let display = document.getElementById("display");
    [...display.children].forEach(col => col.remove());
    for (let x = 0; x < dimensions.width; x++) {
        let column = display.appendChild(elNew("div", { className: "displayColumn" }));
        for (let y = 0; y < dimensions.height; y++) {
            const module = moduleObjects[x * dimensions.height + y];
            let displayElement = column.appendChild(elNew("input", { className: "displayCharacter characterInput", type: "text", maxLength: "1", pattern: "^ $", defaultValue: module.character }));
            displayElement.onbeforeinput = inputChanged;
            displayElement.onclick = function () { this.select(); };
        }
    }
}

async function initialize() {
    startLogWebSocket();
    moduleObjects = await moduleGetAll();

    for (let i = 0; i < moduleObjects.length; i++) {
        moduleObjects[i].command = "motor_unlock";
    }
    modulesSetProperties(["command"]);

    createModuleTable();
    calculateDisplayDimensions();
    createDisplay();
}

function trySetLetter(index, letter) {
    letter = letter.toUpperCase();
    let displayElement = displayCharacterAt(index);
    console.log(letter + " in module " + index + " : " + moduleObjects[index].character_set.includes(letter));
    if (letter.length == 1 && moduleObjects[index].character_set.includes(letter)) {
        moduleObjects[index].character = letter;
        displayElement.value = letter;
        if (['/', '@', ',', ':', ';'].includes(letter)) displayElement.classList.add("characterRaised");
        else displayElement.classList.remove("characterRaised");
        return true;
    }
    //animate
    displayElement.animate([{ color: "#eb3464" }, { color: "" }], 500);
    return false;
}

function inputChanged(e) {
    e.preventDefault()
    switch (e.inputType) {
        case "deleteContentBackward":
        case "deleteWordBackward":
            trySetLetter(activeDisplayIndex(), ' ');
            displayCharacterAt(modulePrev(activeDisplayIndex())).select();
            break;
        case "deleteContentForward":
        case "deleteWordForward":
            trySetLetter(activeDisplayIndex(), ' ');
            displayCharacterAt(moduleNext(activeDisplayIndex())).select();
            break;
        case "insertText":
        case "insertCompositionText":
        case "insertFromPaste":
            let index = activeAnyIndex();
            if (index >= 0) {
                for (let i = 0; i < e.data.length; i++) {
                    if (trySetLetter(index, e.data[i]) || e.data.length > 1) {
                        index = moduleNext(index);
                    }
                };
                displayCharacterAt(index).select();
            } else {
                let acme = activeCharacterSetEntry();
                moduleObjects[acme[0]].character_set[acme[1]] = e.data[0].toUpperCase();
                characterSetEntryAt(acme[0], acme[1]).value = moduleObjects[acme[0]].character_set[acme[1]];
                let nextEntry = characterSetEntryAt(acme[0], acme[1] + 1);
                if (nextEntry) {
                    nextEntry.focus();
                }
            }
            break;
        case "insertLineBreak":
            displayCharacterAt(moduleNext(moduleRowEnd(activeDisplayIndex()))).select();

    }
}

function moduleAbove(index) {
    if (index == Math.floor(index / dimensions.height) * dimensions.height) index += dimensions.height;
    return index - 1;
}
function moduleBelow(index) {
    if (index == (Math.floor(index / dimensions.height) + 1) * dimensions.height - 1) index -= dimensions.height;
    return index + 1;
}
function moduleRight(index) {
    if (Math.floor(index / dimensions.height) + 1 == dimensions.width) index -= dimensions.height * dimensions.width;
    return index + dimensions.height;
}
function moduleLeft(index) {
    if (Math.floor(index / dimensions.height) == 0) index += dimensions.height * dimensions.width;
    return index - dimensions.height;
}
function moduleNext(index) {
    if (Math.floor(index / dimensions.height) + 1 == dimensions.width) index = moduleBelow(index);
    return moduleRight(index);
}
function modulePrev(index) {
    if (Math.floor(index / dimensions.height) == 0) index = moduleAbove(index);
    return moduleLeft(index);
}
function moduleColStart(index) {
    return index - (index % dimensions.height);
}
function moduleColEnd(index) {
    return moduleAbove(moduleColStart(index))
}
function moduleRowStart(index) {
    return index - (Math.floor(index / dimensions.height) * dimensions.height);
}
function moduleRowEnd(index) {
    return moduleLeft(moduleRowStart(index))
}

function keydownHandler(event) {
    const preventDefaultActionKeys = ["Tab", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "PageUp", "PageDown", "Home", "End", "Alt"];
    if (event.defaultPrevented) {
        return;
    }
    if (activeDisplayIndex() + 1 >= 0 && preventDefaultActionKeys.includes(event.key)) {
        event.preventDefault();
    }
    switch (event.key) {
        case "Tab":
            if (activeDisplayIndex() < 0) break;
            if (event.shiftKey) displayCharacterAt(modulePrev(activeDisplayIndex())).select();
            else displayCharacterAt(moduleNext(activeDisplayIndex())).select();
            break;
        case "ArrowUp":
            if (activeDisplayIndex() < 0) break;
            if (event.ctrlKey) displayCharacterAt(moduleColStart(activeDisplayIndex())).select();
            else displayCharacterAt(moduleAbove(activeDisplayIndex())).select();
            break;
        case "ArrowDown":
            if (activeDisplayIndex() < 0) break;
            if (event.ctrlKey) displayCharacterAt(moduleColEnd(activeDisplayIndex())).select();
            else displayCharacterAt(moduleBelow(activeDisplayIndex())).select();
            break;
        case "ArrowLeft":
            if (activeDisplayIndex() < 0) break;
            if (event.ctrlKey) displayCharacterAt(moduleRowStart(activeDisplayIndex())).select();
            else displayCharacterAt(moduleLeft(activeDisplayIndex())).select();
            break;
        case "ArrowRight":
            if (activeDisplayIndex() < 0) break;
            if (event.ctrlKey) displayCharacterAt(moduleRowEnd(activeDisplayIndex())).select();
            else displayCharacterAt(moduleRight(activeDisplayIndex())).select();
            break;
        case "PageUp":
            if (activeDisplayIndex() < 0) break;
            displayCharacterAt(moduleColStart(activeDisplayIndex())).select();
            break;
        case "PageDown":
            if (activeDisplayIndex() < 0) break;
            displayCharacterAt(moduleColEnd(activeDisplayIndex())).select();
            break;
        case "Home":
            if (activeDisplayIndex() < 0) break;
            displayCharacterAt(moduleRowStart(activeDisplayIndex())).select();
            break;
        case "End":
            if (activeDisplayIndex() < 0) break;
            displayCharacterAt(moduleRowEnd(activeDisplayIndex())).select();
            break;
        case "Alt":
            event.preventDefault();
            if (activeDisplayIndex() >= 0) {
                document.activeElement.blur();
            } else {
                displayCharacterAt(0).select();
            }
            break;
        default:
            break;
            return;
    }
}

function filterModuleProperties(properties) {
    allProperties = Object.keys(moduleObjects[0]);
    properties.push("module");
    var deleteProperties = allProperties.filter(function (obj) { return properties.indexOf(obj) == -1; });
    let copy = structuredClone(moduleObjects)
    copy.forEach(element => deleteProperties.forEach(property => (delete element[property])));
    return copy
}

async function modulesSetProperties(properties) {
    const response = await fetch(moduleEndpoint, {
        method: "POST",
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(filterModuleProperties(properties))
    });
}

async function writeNewSetting() {
    json = [];
    for (let i = 0; i < moduleObjects.length; i++) {
        json[i] = moduleSettingsToModule(i);
    }
    const response = await fetch(moduleEndpoint, {
        method: "POST",
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(json)
    });
    initialize();
}

async function writeNewSettingBulk() {
    json = [];
    for (let i = 0; i < moduleObjects.length; i++) {
        json[i] = moduleSettingsToModule(0);
        json[i].module = i;
    }
    const response = await fetch(moduleEndpoint, {
        method: "POST",
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(json)
    });
    initialize();
}

async function uploadFirmware(fileInputId, endpoint) {
    const fileInput = document.getElementById(fileInputId);
    if (!fileInput || !fileInput.files || fileInput.files.length === 0) {
        alert("Please select a firmware .bin file first.");
        return;
    }
    const file = fileInput.files[0];
    try {
        const res = await fetch(endpoint, {
            method: "PUT",
            headers: { 'Content-Type': 'application/octet-stream' },
            body: file
        });
        if (!res.ok) {
            const text = await res.text();
            throw new Error(`Upload failed (${res.status}): ${text}`);
        }
        alert("Firmware upload started successfully.");
    } catch (err) {
        console.error("Firmware upload error:", err);
        alert("Firmware upload error: " + err.message);
    }
}

function updateControllerFirmware() {
    resetProgress('controller');
    uploadFirmware("updateController", controllerFirmwareEndpoint);
}

function updateModuleFirmware() {
    resetProgress('module');
    uploadFirmware("updateModule", moduleFirmwareEndpoint);
}

// -------------------- Firmware Progress (parsed from logs) --------------------
function setProgress(kind, current, total) {
    const bar = document.getElementById(kind === 'module' ? 'progress_module' : 'progress_controller');
    const txt = document.getElementById(kind === 'module' ? 'progress_module_text' : 'progress_controller_text');
    const wrap = txt ? txt.closest('.progressWrap') : null;
    if (!bar || !txt) return;
    const pct = total > 0 ? Math.min(100, Math.max(0, Math.round((current / total) * 100))) : 0;
    // progress bar width: adjust right inset so width == pct%
    bar.style.inset = `0 ${100 - pct}% 0 0`;
    txt.textContent = `${pct}%`;
    if (wrap) {
        wrap.style.display = (pct === 0 || pct === 100) ? 'none' : '';
    }
}

function resetProgress(kind) {
    setProgress(kind, 0, 100);
}

function completeProgress(kind) {
    setProgress(kind, 100, 100);
}

function startCalibration() {
    for (let i = 0; i < moduleObjects.length; i++) {
        trySetLetter(i, moduleObjects[i].character_set[0]);
        moduleObjects[i].offset = 0;
        moduleObjects[i].command = "offset_reset";
    }
    modulesSetProperties(["command"]);
    createModuleTable();
}

function doCalibration() {
    for (let i = 0; i < moduleObjects.length; i++) {
        moduleObjects[i].offset += moduleObjects[i].character_set.indexOf(moduleObjects[i].character);
        if (moduleObjects[i].offset >= moduleObjects[i].character_set.length) moduleObjects[i].offset -= moduleObjects[i].character_set.length; 0
        trySetLetter(i, moduleObjects[i].character_set[0]);
    }
    modulesSetProperties(["offset"]);
}

async function setAccessPoint(type) {
    let ssid = new String(document.getElementById(type + "_ssid").value);
    let pwd = new String(document.getElementById(type + "_pwd").value);
    if (pwd.length < 8 || pwd.length > 63) {
        alert("The password must be between 8 and 63 characters long!");
    } else if (ssid.length > 32) {
        alert("The ssid must be less than 32 characters long!");
    } else {
        const response = await fetch("/api/wifi", {
            method: "POST",
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ [type]: { "ssid": ssid, "password": pwd } })
        });
    }
}

function showCards(cards) {
    [...document.querySelectorAll(".card")].forEach(n => n.style.display = cards.includes(n.id) ? "" : "none");
    // When opening the update card, hide progress wraps at 0% or 100%
    if (cards.includes('update_card')) {
        const wraps = document.querySelectorAll('#update_card .progressWrap');
        wraps.forEach(w => {
            const txt = w.querySelector('.progress__text');
            let pct = 0;
            if (txt && txt.textContent) {
                const m = txt.textContent.match(/(\d+)%/);
                if (m) pct = parseInt(m[1], 10);
            }
            w.style.display = (pct === 0 || pct === 100) ? 'none' : '';
        });
    }
}

function setDefaultcharacterSet() {
    moduleObjects.forEach(module => module.character_set = [" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"]);
}

window.addEventListener("keydown", keydownHandler, true);

let logWS;
let logReconnectTimer;
let logLineBuffer = "";

function colorFromSgr(codes) {
    // codes is an array of numbers like [0,32] or [1,33]
    const hasBright = codes.includes(1) || codes.some(c => c >= 90 && c <= 97);
    const fg = codes.find(c => (c >= 30 && c <= 37) || (c >= 90 && c <= 97));
    const base = {
        30: '#a0a0a0',
        31: '#ff6b6b',
        32: '#51cf66',
        33: '#ffd43b',
        34: '#4dabf7',
        35: '#e599f7',
        36: '#63e6be',
        37: '#f8f9fa',
    };
    const bright = {
        90: '#bfbfbf',
        91: '#ff8787',
        92: '#69db7c',
        93: '#ffe066',
        94: '#74c0fc',
        95: '#f783ff',
        96: '#66d9e8',
        97: '#ffffff',
    };

    if (fg === undefined) return hasBright ? '#ffffff' : 'var(--fg-color)';
    if (fg >= 90) return bright[fg] || '#ffffff';
    const chosen = base[fg] || 'var(--fg-color)';
    if (!hasBright) return chosen;
    return chosen;
}

function stripAnsi(line) {
    return line.replace(/\x1b\[[0-9;]*m/g, '');
}

function parseLineColor(line) {
    const m = line.match(/^\x1b\[([0-9;]+)m/);
    if (!m) return undefined;
    const codes = m[1].split(';').map(n => parseInt(n, 10)).filter(n => !Number.isNaN(n));
    return colorFromSgr(codes);
}

function appendLogLine(line, forcedColor) {
    const el = document.getElementById('log_window');
    if (!el) return;
    const shouldStick = (el.scrollTop + el.clientHeight + 10) >= el.scrollHeight;

    // Normalize CRLF/CR
    line = line.replace(/\r$/, '');

    const color = forcedColor || parseLineColor(line);
    const clean = stripAnsi(line);

    // Detect firmware progress lines
    // Module example: "MODULE_FIRMWARE_ENDPOINTS: writing 128 5888/28672 bytes"
    const mod = clean.match(/MODULE_FIRMWARE_ENDPOINTS:\s*writing\s+\d+\s+(\d+)\/(\d+)\s+bytes/i);
    if (mod) {
        const cur = parseInt(mod[1], 10);
        const tot = parseInt(mod[2], 10);
        if (!Number.isNaN(cur) && !Number.isNaN(tot)) {
            setProgress('module', cur, tot);
            if (cur >= tot) completeProgress('module');
        }
    }

    // Controller progress (future-proof): look for CONTROLLER_FIRMWARE lines of same shape
    const ctl = clean.match(/CONTROLLER_OTA.*?:\s*writing\s+\d+\s+(\d+)\/(\d+)\s+bytes/i);
    if (ctl) {
        const cur = parseInt(ctl[1], 10);
        const tot = parseInt(ctl[2], 10);
        if (!Number.isNaN(cur) && !Number.isNaN(tot)) {
            setProgress('controller', cur, tot);
            if (cur >= tot) completeProgress('controller');
        }
    }

    const span = document.createElement('span');
    span.textContent = clean;
    span.style.display = 'block';
    if (color) span.style.color = color;
    el.appendChild(span);

    if (shouldStick) el.scrollTop = el.scrollHeight;
}

function appendLogChunk(text) {
    // Accumulate and flush per-line
    logLineBuffer += text;
    let idx;
    while ((idx = logLineBuffer.indexOf('\n')) !== -1) {
        const line = logLineBuffer.slice(0, idx);
        logLineBuffer = logLineBuffer.slice(idx + 1);
        if (line.length === 0) { appendLogLine(''); } else { appendLogLine(line); }
    }
}

function appendStatusLine(text) {
    appendLogLine(text, '#bfbfbf');
}

function startLogWebSocket() {
    const url = `ws://${location.host}/log`;

    try {
        if (logWS) {
            try { logWS.close(); } catch (_) { }
        }
        logWS = new WebSocket(url);
    } catch (e) {
        console.error("WebSocket init error:", e);
        scheduleLogReconnect();
        return;
    }

    logWS.onopen = () => {
        clearTimeout(logReconnectTimer);
        appendStatusLine("[log] connected");
    };

    logWS.onmessage = (ev) => {
        // Accept text frames; if binary, try to decode as UTF-8
        if (typeof ev.data === 'string') {
            appendLogChunk(ev.data);
        } else if (ev.data instanceof Blob) {
            ev.data.text().then(appendLogChunk).catch(() => { });
        } else if (ev.data instanceof ArrayBuffer) {
            try { appendLogChunk(new TextDecoder().decode(ev.data)); } catch (_) { }
        }
    };

    logWS.onerror = (ev) => {
        console.error("WebSocket error:", ev);
    };

    logWS.onclose = () => {
        appendStatusLine("[log] disconnected, reconnecting...");
        scheduleLogReconnect();
    };
}

function scheduleLogReconnect() {
    clearTimeout(logReconnectTimer);
    logReconnectTimer = setTimeout(startLogWebSocket, 2000);
}
