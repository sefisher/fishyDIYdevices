// This is the template file for common JS scripts used by most fishyDIY device webpages.
// To build this template file intp the code after updating it do the following:
//  1) select all and copy this file (Ctrl-a ctrl-a on windows)
//  2) put it through a minifier (like https://kangax.github.io/html-minifier/ with "Decode Entity Characters OFF") and 
//  then through a String builder (like http://buildmystring.com/ with "strip out carriage returns" on) to create a 
//  single string with no tabs and one line.
//  3) paste that string into the WEBSTR_COMMON_JS After "PROGMEM" in the fishyDevices.h file.
//-----
// (NOTE: to run this .js file through https://kangax.github.io/html-minifier/ you'll need to enclose this file contents 
// in <script></script> tags - it isn't set up to recognize javascript without that. Then REMOVE the 
// <script></script> before pasting into http://buildmystring.com/).
//-----

//websocket variable
var websock;

//utility function to test for getElementById
function _(el) {
    return document.getElementById(el);
}

//utility function to test for a compatible browser
function alertBadBrowser(){
    var isIE = /*@cc_on!@*/false || !!document.documentMode;
    var isEdge = !isIE && !!window.StyleMedia;

    if(isIE||isEdge){
        alert("Your browser may cause display problems. You should obtain a modern webkit browser.  While some versions of Microsoft Edge work, it can be buggy. Chrome, Firefox, Safari, and Opera all work consistently. Internet Explorer is not supported.");
    }
}
//utility function for all fishyDIY devices (updates master switch on toggle)
function swMstrUpd() {
    var label = _('swMstrLab');
    var sw = _('swMstrChck');
    if (sw.checked == true) { label.innerHTML = 'Master Node'; } else { label.innerHTML = 'Slave Node'; }
}
//utility function for all fishyDIY devices (called on to restore normal background color after a message is sent)
function returnColor(){
    document.body.style.backgroundColor = "var(--bg-col)";
}
//utility function for all fishyDIY devices (takes the device status info - parses to a JSON object and sends on to do stuff)
function processJSON(JSONstr){
    var data = JSON.parse(JSONstr);
    doStuffWithJSON(data.fishyDevices);
}
//utility function for all fishyDIY devices 
//('Only letters, numbers, underscore, space, comma, period, and dash are allowed.');
function blockSpecialChar(e) {
    var k = e.keyCode;
    return ((k > 64 && k < 91) || (k > 96 && k < 123) || k == 8 || k == 16 || k == 95  || k == 32 || (k > 43 && k < 47)  || (k >= 48 && k <= 57));
}
//utility function for all fishyDIY devices 
//('Only numbers are allowed.');
function numberCharOnly(e) {
    var k = e.keyCode;
    return (k == 8 || k == 16 || (k >= 48 && k <= 57));
}
//utility function for all fishyDIY devices (replaces jquery function)
function loadJSON(path, success, error){
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function()
    {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            if (xhr.status === 200) {
                if (success)
                    success(JSON.parse(xhr.responseText));
            } else {
                if (error)
                    error(xhr);
            }
        }
    };
    xhr.open("GET", path, true);
    xhr.send();
}
//utility function for all fishyDIY devices (called on to open infoText Modal with device data)
function showDetails() {
    _('infoDiv').innerHTML = infoText;
    _('infoPanel').style.display = 'block';
}
//utility function for all fishyDIY devices (extract after MSG:and before ~*~*)
function getMsg(data) {
    var start = data.indexOf("MSG:") + 4;
    var end = data.indexOf("~*~*");
    return data.substring(start, end);
}
//utility function for all fishyDIY devices (extract after ~*~*DAT):
function getNodeJSONtext(data) {
    var start = data.indexOf("~*~*DAT:") + 8;
    return data.substring(start, data.length);
}
//utility function for all fishyDIY devices (extract after ~*~*DAT):
function start() {
    // TEST - TODO turn off when not testing
    //if(document.domain=="localhost"){websock = new WebSocket('ws://10.203.1.197/ws');}
    if (document.domain == "localhost"){websock = new WebSocket('ws://10.203.1.23/ws')}
    else { 
        websock = new WebSocket('ws://' + window.location.hostname + '/ws'); 
    };

    websock.onopen = function(evt) { 
        console.log('websock open'); 
    };
    websock.onclose = function(evt) { 
        console.log('websock close');    
        alert("This control panel is no longer connected to the device.  Please close this window and reopen the control panel.");
        return 0;
    };                
    websock.onerror = function (evt) { console.log(evt); };
    websock.onmessage = function (evt) {

        var payload = evt.data;
        if (payload.indexOf("~*~*") < 0) {
            if (payload.indexOf('{"fishyDevices"') == 0) {
                processJSON(payload);
            } else {
                console.log(payload);
            }
        } else {
            dealWithMessagePayload(payload);
        }
    };
}