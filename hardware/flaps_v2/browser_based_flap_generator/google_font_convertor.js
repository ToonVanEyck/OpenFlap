/**
 * opentype.js helper
 * Based on @yne's comment
 * https://github.com/opentypejs/opentype.js/issues/183#issuecomment-1147228025
 * will decompress woff2 files
 */

async function loadFont(src) {
    let buffer = {};
    let font = {};
    let ext = 'woff2';
    let url;

    // 1. is file
    if (src instanceof Object) {
        // get file extension to skip woff2 decompression
        let filename = src.name.split(".");
        ext = filename[filename.length - 1];
        buffer = await src.arrayBuffer();
    }

    // 2. is base64 data URI
    else if (/^data/.test(src)) {
        // is base64
        let data = src.split(";");
        ext = data[0].split("/")[1];

        // create buffer from blob
        let srcBlob = await (await fetch(src)).blob();
        buffer = await srcBlob.arrayBuffer();
    }

    // 3. is url
    else {
        // if google font css - retrieve font src
        if (/googleapis.com/.test(src)) {
            ext = 'woff2';
            src = await getGoogleFontUrl(src);
        }

        // might be subset - no extension
        let hasExt = (src.includes('.woff2') || src.includes('.woff') || src.includes('.ttf') || src.includes('.otf')) ? true : false;
        url = src.split(".");
        ext = hasExt ? url[url.length - 1] : 'woff2';

        let fetchedSrc = await fetch(src);
        buffer = await fetchedSrc.arrayBuffer();
    }
    // decompress woff2
    if (ext === "woff2") {
        buffer = Uint8Array.from(Module.decompress(buffer)).buffer;
    }

    // parse font
    font = opentype.parse(buffer);
    return font;
}



/**
 * load google font subset
 * containing all glyphs used in document
 */
async function getGoogleFontSubsetFromContent(url, documentText = '') {

    // get all characters used in body
    documentText = documentText ? documentText : document.body.innerText.trim()
        .replace(/[\n\r\t]/g, "")
        .replace(/\s{2,}/g, " ")
        .replaceAll(' ', "");
    url = url + '&text=' + encodeURI(documentText);

    let fetched = await fetch(url);
    let res = await fetched.text();
    let src = res.match(/[^]*?url\((.*?)\)/)[1];

    return src;
}


/**
 * load fonts from google helper
 */
async function getGoogleFontUrl(url, subset = "latin", documentText = '') {
    let src;
    // get subset based on used characters
    if (documentText) {
        src = getGoogleFontSubsetFromContent(url, documentText);
        return src;
    }
    let fetched = await fetch(url);
    let res = await fetched.text();

    // get language subsets
    let subsetObj = {};
    let subsetRules = res.split("/*").filter(Boolean);

    for (let i = 0; i < subsetRules.length; i++) {
        let subsetRule = subsetRules[i];
        let rule = subsetRule.split("*/");
        let subset = rule[0].trim();
        let src = subsetRule.match(/[^]*?url\((.*?)\)/)[1];
        subsetObj[subset] = src;
    }

    src = subsetObj[subset];
    return src;
}

