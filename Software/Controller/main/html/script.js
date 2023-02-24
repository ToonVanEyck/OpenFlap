
// const moduleEndpoint = "http://openflap.local/api/modules"
const moduleEndpoint = "/api/modules"

var moduleObjects = [];
var dimensions = {width:0,height:0};

async function moduleGetAll(){
    const response = await fetch(moduleEndpoint, {
        method:"GET",
        headers:{"Content-Type": "application/json"}
    });
    return await response.json();
}

const elNew = (tag, prop) => Object.assign(document.createElement(tag), prop);

var activeDisplayIndex = () => [...document.querySelectorAll(".displayCharacter")].indexOf(document.activeElement)
var displayCharacterAt = (index) => [...document.querySelectorAll(".displayCharacter")][index]
var activePropertyIndex = () => [...document.querySelectorAll(".propertyCharacter")].indexOf(document.activeElement)
var propertyCharacterAt = (index) => [...document.querySelectorAll(".propertyCharacter")][index]
var activeAnyIndex = () => activeDisplayIndex() + activePropertyIndex() +1;

function createModuleTable(){
    let table = document.getElementById("moduleTable");
    table.children[0].remove();
    table = table.appendChild(elNew("tbody"));
    for (let i = 0; i < moduleObjects.length; i++) {
        const module = moduleObjects[i];
        let row = table.appendChild(elNew("tr")).appendChild(elNew("td")).appendChild(elNew("div",{className: "moduleConfig"}));
        // module character
        let moduleCharacter = row.appendChild(elNew("div",{className: "moduleCharacter"}));
        moduleCharacter.appendChild(elNew("div",{className: "moduleConfigTitle"})).append(elNew("div",{innerHTML:"Module "+i+":"}));
        moduleCharacter.appendChild(elNew("div",{className: "contentWrapper"})).append(elNew("input",{type:"text", className:"propertyCharacter characterInput",pattern:"^ $",maxLength:"1",defaultValue:module.character}));
        moduleCharacter.lastChild.lastChild.onclick = function() {this.select();};
        // module properties
        let moduleProperties = row.appendChild(elNew("div",{className: "moduleProperties"}));
        moduleProperties.appendChild(elNew("div",{className: "moduleConfigTitle"})).append(elNew("div",{innerHTML:"Properties:"}));
        let propertyList = moduleProperties.appendChild(elNew("div",{className: "contentWrapper"})).appendChild(elNew("div",{className:"moduleProperties__list"}));
        propertyList.appendChild(elNew("div",{className: "moduleProperties__entry"})).append(elNew("label",{htmlFor: "columnEnd_"+i,innerHTML:"columnEnd"}),elNew("input",{id: "columnEnd_"+i,className:"propertyInput checkBox noclick", type:"checkbox",name:"columnEnd",checked:module.columnEnd}));
        propertyList.appendChild(elNew("div",{className: "moduleProperties__entry"})).append(elNew("label",{htmlFor: "vtrim_"+i, innerHTML:"vtrim"}),elNew("input",{id: "vtrim_"+i,className:"propertyInput", type:"number",min:"0", max:"200", step:"5",name:"vtrim",defaultValue:module.vtrim}));
        propertyList.lastChild.lastChild.onchange = function(){module.vtrim = this.value};
        propertyList.lastChild.lastChild.onclick = function() {this.select();};
        propertyList.appendChild(elNew("div",{className: "moduleProperties__entry"})).append(elNew("label",{htmlFor: "offset_"+i, innerHTML:"offset"}),elNew("input",{id: "offset_"+i,className:"propertyInput", type:"number",min:"0", max:module.characterMap.length-1,name:"offset",defaultValue:module.offset}));
        propertyList.lastChild.lastChild.onchange = function(){module.offset = this.value};
        propertyList.lastChild.lastChild.onclick = function() {this.select();};
        // module characterMap
        let moduleCharaterMap = row.appendChild(elNew("div",{className: "moduleCharacterMap"}));
        moduleCharaterMap.appendChild(elNew("div",{className: "moduleConfigTitle"})).append(elNew("div",{innerHTML:"Supported Characters:"}));
        let characterMap = moduleCharaterMap.appendChild(elNew("div",{className: "contentWrapper"})).appendChild(elNew("div",{className:"characterMap"}));
        for(let j = 0; j<module.characterMap.length; j++){
            characterMap.appendChild(elNew("input",{id: "characterMapMember_"+i+"_"+j,className:"characterMapMember characterInput", pattern:"^ $", type:"text",maxLength:"1",defaultValue:module.characterMap[j]}));
            characterMap.lastChild.onchange = function() {module.characterMap[j] = this.value.toUpperCase();};
            characterMap.lastChild.onclick = function() {this.select();};
        }
    }
}

function calculateDisplayDimensions(){
    var width = 0;
    moduleObjects.forEach(module => width += (module.columnEnd));
    var height = moduleObjects.length / width;
    if(!Number.isInteger(height)){
        console.log("The display is not rectangular!");
        return
    }
    dimensions.height = height;
    dimensions.width = width
    let root = document.querySelector(':root');
    root.style.setProperty('--dispWidth', width);
    root.style.setProperty('--dispHeight', height);
}

function createDisplay(){
    let display = document.getElementById("display");
    [...display.children].forEach(col => col.remove());
    for (let x = 0; x < dimensions.width; x++) {
        let column = display.appendChild(elNew("div",{className: "displayColumn"}));
        for (let y = 0; y < dimensions.height; y++) {
            const module = moduleObjects[x*dimensions.height + y];
            let displayElement = column.appendChild(elNew("input",{className: "displayCharacter characterInput",type:"text",maxLength:"1", pattern:"^ $" ,defaultValue:module.character}));
            displayElement.onchange = function(){module.character = this.value.toUpperCase();}
            displayElement.onclick = function() {this.select();};
        }
    }
}  

async function initialize(){
    moduleObjects = await moduleGetAll();
    createModuleTable();
    calculateDisplayDimensions();
    createDisplay();
}

function trySetLetter(index,letter){
    letter = letter.toUpperCase();
    let displayElement = displayCharacterAt(index);
    let propertyElement = propertyCharacterAt(index);
    if(letter.length == 1 && moduleObjects[index].characterMap.includes(letter)){
        moduleObjects[index].character = letter;
        displayElement.value = letter;
        if(['/','@',',',':',';'].includes(letter)) displayElement.classList.add("characterRaised");
        else displayElement.classList.remove("characterRaised");
        propertyElement.value = letter;
        if(['/','@',',',':',';'].includes(letter)) propertyElement.classList.add("characterRaised");
        else propertyElement.classList.remove("characterRaised");
        return true;
    }
    //animate
    displayElement.animate([{color:"#eb3464"},{color:""}], 500);
    propertyElement.animate([{color:"#eb3464"},{color:""}], 500);
    return false;
}

function moduleAbove(index){
    if(index == Math.floor(index/dimensions.height) * dimensions.height) index += dimensions.height;
    return index-1;
}
function moduleBelow(index){
    if(index == (Math.floor(index/dimensions.height)+1) * dimensions.height -1) index -= dimensions.height;
    return index+1;
}
function moduleRight(index){
    if(Math.floor(index/dimensions.height)+1 == dimensions.width) index -= dimensions.height * dimensions.width;
    return index+dimensions.height;
}
function moduleLeft(index){
    if(Math.floor(index/dimensions.height) == 0) index += dimensions.height * dimensions.width;
    return index-dimensions.height;
}
function moduleNext(index){
    index = moduleRight(index);
    if(Math.floor(index/dimensions.height)+1 == dimensions.width) index = moduleBelow(index);
    return index;
}
function modulePrev(index){
    if(Math.floor(index/dimensions.height) == 0) index = moduleAbove(index);
    return moduleLeft(index);
}
function moduleColStart(index){
    return index - (index % dimensions.height);
}
function moduleColEnd(index){
    return moduleAbove(moduleColStart(index))
}
function moduleRowStart(index){
    return index - (Math.floor(index/dimensions.height) * dimensions.height);
}
function moduleRowEnd(index){
    return moduleLeft(moduleRowStart(index))
}

window.addEventListener("keydown", function (event) {
    if (event.defaultPrevented) {
        return; 
    }

        if(activeDisplayIndex() + activePropertyIndex() + 1 >= 0){
            event.preventDefault();
        }
        switch (event.key) {
            case "Tab":
                if(activeDisplayIndex() < 0) break;
                if(event.shiftKey)  displayCharacterAt(modulePrev(activeDisplayIndex())).select();
                else                displayCharacterAt(moduleNext(activeDisplayIndex())).select();
            break;
            case "ArrowUp":
                if(activeDisplayIndex() < 0) break;
                if(event.ctrlKey)   displayCharacterAt(moduleColStart(activeDisplayIndex())).select();
                else                displayCharacterAt(moduleAbove(activeDisplayIndex())).select();
                break;
            case "ArrowDown":
                if(activeDisplayIndex() < 0) break;
                if(event.ctrlKey)   displayCharacterAt(moduleColEnd(activeDisplayIndex())).select();
                else                displayCharacterAt(moduleBelow(activeDisplayIndex())).select();
                break;
            case "ArrowLeft":
                if(activeDisplayIndex() < 0) break;
                if(event.ctrlKey)   displayCharacterAt(moduleRowStart(activeDisplayIndex())).select();
                else                displayCharacterAt(moduleLeft(activeDisplayIndex())).select();
                break;
            case "ArrowRight":
                if(activeDisplayIndex() < 0) break;
                if(event.ctrlKey)   displayCharacterAt(moduleRowEnd(activeDisplayIndex())).select();
                else                displayCharacterAt(moduleRight(activeDisplayIndex())).select();
                break;
            case "PageUp":
                if(activeDisplayIndex() < 0) break;
                displayCharacterAt(moduleColStart(activeDisplayIndex())).select();
                break;
            case "PageDown":
                if(activeDisplayIndex() < 0) break;
                displayCharacterAt(moduleColEnd(activeDisplayIndex())).select();
                break;
            case "Home":
                if(activeDisplayIndex() < 0) break;
                displayCharacterAt(moduleRowStart(activeDisplayIndex())).select();
                break;
            case "End":
                if(activeDisplayIndex() < 0) break;
                displayCharacterAt(moduleRowEnd(activeDisplayIndex())).select();
                break;
            case "Backspace":
                trySetLetter(activeDisplayIndex() + activePropertyIndex() + 1,' ');
                displayCharacterAt(modulePrev(activeDisplayIndex())).select();
                break;
            case "Delete":
                if(activeAnyIndex >= 0){
                    trySetLetter(activeDisplayIndex() + activePropertyIndex() + 1,' ');
                    displayCharacterAt(moduleNext(activeDisplayIndex())).select();
                }else{
                    for(let i = 0; i < moduleObjects.length; trySetLetter(i++,' '));
                };
                break;
            case "Enter":
                if(activeDisplayIndex() >= 0){
                    modulesSetProperties(["character"])();
                    document.activeElement.blur();
                }
                break;
            case "Alt":
                event.preventDefault();
                if(activeDisplayIndex() >= 0){
                    document.activeElement.blur();
                }else{
                    displayCharacterAt(0).select();         
                }
                break;
            default:  
                if(event.key.length>1) break;
                if(trySetLetter(activeDisplayIndex() + activePropertyIndex() + 1,event.key) && activeDisplayIndex() >= 0) displayCharacterAt(moduleNext(activeDisplayIndex())).select();
                break;
            return;
        }
}, true);

function filterModuleProperties(properties){
    allProperties = Object.keys(moduleObjects[0]);
    properties.push("module");
    var deleteProperties = allProperties.filter(function(obj) { return properties.indexOf(obj) == -1; });
    let copy = structuredClone(moduleObjects)
    copy.forEach(element => deleteProperties.forEach(property => (delete element[property])));
    return copy
}

async function modulesSetProperties(properties){
    const response = await fetch(moduleEndpoint, {
        method:"POST",
        headers:{'Content-Type': 'application/json'},
        body: JSON.stringify(filterModuleProperties(properties))
    });
}

function startCalibration(){
    for(let i = 0; i < moduleObjects.length; i++){
        trySetLetter(i,moduleObjects[i].characterMap[0]);
        moduleObjects[i].offset = 0;
        moduleObjects[i].vtrim = 0;
    }
    modulesSetProperties(["character", "offset", "vtrim"]);
    createModuleTable();
}

function doCalibration(){
    for(let i = 0; i < moduleObjects.length; i++){
        moduleObjects[i].offset += moduleObjects[i].characterMap.indexOf(moduleObjects[i].character);
        if(moduleObjects[i].offset >= moduleObjects[i].characterMap.length) moduleObjects[i].offset -= moduleObjects[i].characterMap.length;0
        trySetLetter(i,moduleObjects[i].characterMap[0]);
    } 
    modulesSetProperties(["character", "offset"]);
}

async function setAccessPoint(type){
    let ssid = new String(document.getElementById(type+"_ssid").value);
    let pwd = new String(document.getElementById(type+"_pwd").value);
    if(pwd.length < 8 || pwd.length > 63){
        alert("The password must be between 8 and 63 characters long!");
    }else if(ssid.length > 32){
        alert("The ssid must be less than 32 characters long!");
    }else{
        const response = await fetch("/api/wifi", {
            method:"POST",
            headers:{'Content-Type': 'application/json'},
            body: JSON.stringify({[type]:{"ssid":ssid,"password":pwd}})
        });
    }
}

function showCards(cards){
    [...document.querySelectorAll(".card")].forEach(n => n.style.display = cards.includes(n.id)?"":"none")
}