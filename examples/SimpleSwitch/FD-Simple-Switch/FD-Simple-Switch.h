//=============================================================================
//                  Fishy Device Limit Switch Actuator
//=============================================================================
// CUSTOM GLOBALS AND TYPES
//------------------------DEVICE TYPE SETTINGS------------------------------//
//ENTER THIS DEVICE'S TYPE USING ONLY LETTERS AND NUMBERS (NO SPACES)
//LIMITED to 20 alpanumeric characters
extern const char CUSTOM_DEVICE_TYPE[] = "Simple-Switch";

//This says EEPROM since it is extracted from the 255 char (max) string stored in the fD.myEEPROMdata struct
//that is stored in EEPROM.  This struct is dynamic (not stored), but is encoded into the char[] then stored.
struct EEPROMdeviceData
{
    bool power_state[NUM_SWITCHES];
    char otherNames[3][MAXNAMELEN] = {"","",""};
} EEPROMdeviceData;

Bounce debouncer[NUM_SWITCHES] = Bounce(); // Instantiate a Bounce object for debouncing switch

//=============================================================================

//=============================================================================
/*  DECLARATIONS -->

CUSTOM DEVICE FUNCTIONS - EXTERNAL - The following 
COMMON EXTERNAL functions are required for all fishyDevice objects are DECLARED in
fishyDevice.h but must be DEFINED in this file using the fishyDevice namespace (fishyDevice::).

void operateDevice();
void deviceSetup(); 
void executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote);
void executeState(unsigned char, const char*,bool state, unsigned char, context int); 
void UDPparseConfigResponse(String responseIn, IPAddress remote);
String getStatusString();
String getShortStatString();
void initializeDeviceCustomData();
void extractDeviceCustomData();
void encodeDeviceCustomData();
void showEEPROMdevicePersonalityData();
bool isCustomDeviceReady();
String getDeviceSpecificJSON();

The declarations immediately following are for non-COMMON functions (unique to this device) 
defined in this file:  */


//=============================================================================
// hardware assumed here is ESP8266 (ESP-12E) connected to relay board. 
// connect a switch to GPIO3 in input_pullup mode. Note that this uses the RX 
// pin on the ESP-01 so you can't use the serial input. Setting for 
// "USE_SERIAL_INPUT" in fishyDevices.h must be set to FALSE to use GPIO 3
//=============================================================================

// NODE MCU PIN MAPPING
//see https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html for notes/issues with each GPIO
// static const uint8_t D0   = 16; //no internal pullup resistor - has internal pull_down resistor
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;  
// static const uint8_t D8   = 15; //internal pull_up resistor not usable, can't power during software loading
// static const uint8_t D9   = 3;  //RECEIVE not usable w/ serial comms unless USE_SERIAL_INPUT = FALSE (fishyDevices.h)
// static const uint8_t D10  = 1;  //TRANSMIT not usable w/ serial comms

// pin definitions - THESE ARE FOR ESP8266 ESP-12E (Node MCU 1.0)
#define RELAY1_PIN 16 // D0 = GPIO16 for relay output
#define RELAY2_PIN 12 // D6 = GPIO12 for relay output 
#define RELAY3_PIN 13 // D7 = GPIO13 for relay output 
#define RELAY4_PIN 3  // D9 = GPIO3 for relay output (USE_SERIAL_INPUT = FALSE - ok to use)

#define PWRSW1_PIN 5  // D1 = GPIO5 for button input
#define PWRSW2_PIN 4  // D2 = GPIO4 for button input
#define PWRSW3_PIN 0  // D3 = GPIO0 for button input
#define PWRSW4_PIN 2  // D4 = GPIO2 for button input

const int relayPin[] = {RELAY1_PIN,RELAY2_PIN,RELAY3_PIN,RELAY4_PIN};
const int pwrSwPin[] = {PWRSW1_PIN,PWRSW2_PIN,PWRSW3_PIN,PWRSW4_PIN};
//------------------------------------------------------------------


//=============================================================================================================================
//CTRL SITE PARTS - Used to serve up each device's controls
//WEBCTRLSTR is common to all devices and is REQUIRED
const char WEBCTRLSTR[] PROGMEM = "<!doctypehtml><title>fishDIY Simple-Switch</title><meta content=\"initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css id=styles rel=stylesheet><div class=main id=myBody><script>function dealWithMessagePayload(e){var t=getMsg(e);console.log(t),setOtherTxt(t);var n=getNodeJSONtext(e);console.log(n),processJSON(n)}function setMainTxt(e){_(\"mainTxt\").innerHTML=e}function setOtherTxt(e){_(\"otherTxt\").innerHTML=e}function swPwrUpd(e){var t=_(\"swPwr\"+e+\"Lab\");1==_(\"swPwr\"+e+\"Chck\").checked?(t.innerHTML=\"On\",sendCmd(\"on\"+e)):(t.innerHTML=\"Off\",sendCmd(\"off\"+e))}function sendCmd(e){if(1!=websock.readyState)return alert(\"This control panel is no longer connected to the device.  Please close this window and reopen the control panel.\"),0;var t=\"\";if(\"o\"==e.substring(0,1))t=e;else if(\"reboot\"==e)t=\"reset\";else if(\"rstWifi\"==e)t=\"reset_wifi\";else if(\"config\"==e){var n=\"\",s=\"false\";for(_(\"swMstrChck\").checked&&(s=\"true\"),t=\"config\",t+=\";isMaster=\"+s,i=1;i<=4;i++)swNameStr=\"sw\"+i+\"Name\",n=\"\"==_(swNameStr).value?\" \":_(swNameStr).value,t+=\";\"+swNameStr+\"=\"+n;t+=\";note=\"+(n=\"\"==_(\"devNote\").value?\" \":_(\"devNote\").value),t+=\";timeOut=\"+(n=\"\"==_(\"timeOut\").value?\"60\":_(\"timeOut\").value)}document.body.style.backgroundColor=\"var(--bg-col-ack)\",websock.send(t)}var infoText=\"...Loading...\";function doStuffWithJSON(e){var t=\"\",n=!1,s=\"\";for(d=e[0],returnColor(),\"true\"==d.isMaster?(t=\" (MASTER)\",_(\"swMstrChck\").checked=!0):(t=\"\",_(\"swMstrChck\").checked=!1),\"true\"==d.deviceTimedOut&&(n=!0,s=\"ERROR - DEVICE TIMED OUT\\n\"),swMstrUpd(),infoText=\"DEVICE TYPE: \"+d.devType+\"<br>\\nNAME: \"+d.names[1]+\"<br>\\nIP ADDRESS: \"+d.ip+t+\"<hr>\\nSOFTWARE VERSION: \"+d.swVer+\"<br>\\nINIT STR: \"+d.initStamp+\"\\n\\n\\n<hr><br>\\n\"+s,_(\"info-icon\").className=n?\"info-icon-blink\":\"info-icon\",_(\"devIP\").value=d.ip+t,_(\"devNote\").value=d.note,_(\"timeOut\").value=d.timeOut,numSw=d.NumSwitches,mainTxt=\"\",i=1;i<=numSw;i++)swNameStr=\"sw\"+i+\"Name\",_(swNameStr).value=d.names[i],swNameStr=\"sw\"+i+\"NameCtrl\",_(swNameStr).innerHTML=d.names[i],mainTxt=mainTxt+i+\"-\"+pos(d.power_state[i])+\" \",swNameStr=\"swPwr\"+i+\"Chck\",labStr=\"swPwr\"+i+\"Lab\",\"1\"==d.power_state[i]?(_(swNameStr).checked=!0,_(labStr).innerHTML=\"On\"):(_(swNameStr).checked=!1,_(labStr).innerHTML=\"Off\");for(setMainTxt(mainTxt),numSw<3&&(_(\"swGrid2\").style.display=\"none\");i<=4;i++)swNameStr=\"sw\"+i+\"Row\",_(swNameStr).style.display=\"none\",swNameStr=\"sw\"+i+\"LabelSpan\",_(swNameStr).style.display=\"none\",swNameStr=\"sw\"+i+\"Name\",_(swNameStr).value=\"\";_(\"devType\").value=d.devType}function pos(e){return\"1\"==e?\"On\":\"Off\"}window.onload=start()</script></div><div class=flex-grid-outer id=flex-container><div class=fishyHdr id=deviceStatus><table width=100%><tr><td><div class=info-icon id=info-icon onclick=showDetails()>&#9432;</div><td><div class=flex-grid style=padding:6px><table><tr><td style=font-size:12px>Power:<tr><td id=mainTxt>--</table><span class=otherTxt id=otherTxt></span></div></table></div><div><div class=swPanel><div class=flex-grid id=swGrid1 style=padding:6px;justify-content:space-around;flex-wrap:wrap><div class=swRow id=sw1Row style=text-align:center><div id=sw1NameCtrl>Name 1</div><label class=sw id=swPwr1><input id=swPwr1Chck type=checkbox onchange=swPwrUpd(1)><span class=sldSw></span></label><span class=swLab id=swPwr1Lab style=font-size:larger;top:0>Off</span></div><div class=swRow id=sw2Row style=text-align:center><div id=sw2NameCtrl>Name 2</div><label class=sw id=swPwr2><input id=swPwr2Chck type=checkbox onchange=swPwrUpd(2)><span class=sldSw></span></label><span class=swLab id=swPwr2Lab style=font-size:larger;top:0>Off</span></div></div><div class=flex-grid id=swGrid2 style=padding:6px;justify-content:space-around;flex-wrap:wrap><div class=swRow id=sw3Row style=text-align:center><div id=sw3NameCtrl>Name 3</div><label class=sw id=swPwr3><input id=swPwr3Chck type=checkbox onchange=swPwrUpd(3)><span class=sldSw></span></label><span class=swLab id=swPwr3Lab style=font-size:larger;top:0>Off</span></div><div class=swRow id=sw4Row style=text-align:center><div id=sw4NameCtrl>Name 4</div><label class=sw id=swPwr4><input id=swPwr4Chck type=checkbox onchange=swPwrUpd(4)><span class=sldSw></span></label><span class=swLab id=swPwr4Lab style=font-size:larger;top:0>Off</span></div></div></div></div><div class=configPanel><label class=cfgInpLab for=devIP>IP Address:<input id=devIP class=cfgInp value=...Loading... maxlength=40 disabled></label><br><label class=cfgInpLab for=sw1Name>SW1 Name:<input id=sw1Name class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br><span id=sw2LabelSpan><label class=cfgInpLab for=sw2Name>SW2 Name:<input id=sw2Name class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br></span><span id=sw3LabelSpan><label class=cfgInpLab for=sw3Name>SW3 Name:<input id=sw3Name class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br></span><span id=sw4LabelSpan><label class=cfgInpLab for=sw4Name>SW4 Name:<input id=sw4Name class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br></span><label class=cfgInpLab for=devType>Type:<input id=devType class=cfgInp value=...Loading... maxlength=20 disabled></label><br><label class=cfgInpLab for=devNote>Note:<input id=devNote class=cfgInp value=...Loading... maxlength=55 onkeypress=\"return blockSpecialChar(event)\"></label><br><label class=cfgInpLab for=timeOut>Time-Out:<input id=timeOut class=cfgInpNum value=00 maxlength=5 onkeypress=\"return numberCharOnly(event)\"title=\"Enter a time (in seconds) to wait for the actuator to respond before stopped and displaying an error message (assuming the actuator got stuck).\"></label></div><div class=swPanel><div class=flex-grid><div class=swRow><label class=sw id=swMstr><input id=swMstrChck type=checkbox onchange=swMstrUpd() checked><span class=sldSw></span></label><span class=swLab id=swMstrLab style=font-size:larger;top:0>Master Node</span></div></div></div><div class=fishyFtrSw><div class=flex-grid><input id=updCfgBtn class=cfgbuttonY value=\"UPDATE SETTINGS\"title=\"This will save new settings in the device. New settings should be displayed on next refresh.  If trying to adjust mulitple settings it is recommended that you temporarily turn off Auto-refresh.\"onclick='sendCmd(\"config\")'type=button> <input id=updCfgBtn class=cfgbuttonY value=\"REBOOT DEVICE\"title=\"This will reboot the device.\"onclick='sendCmd(\"reboot\")'type=button></div><div class=flex-grid><input id=rstWifiBtn class=cfgbuttonR value=\"RESET WIFI\"title=\"WARNING:Reset Wifi will attempt to delete your network SSID and passwordand put the device into wifi-server mode to learn new wifi SSID and password. To teach a reset device a new wifi network go to the device IP address using a mobile phone or computer.\"onclick='sendCmd(\"rstWifi\")'type=button></div></div></div><div class=infoPanel id=infoPanel><div class=infoDiv id=infoDiv></div><input class=infoOKbtn value=OK onclick=closeCtrlModal() type=button></div><script>var ctrlModal=_(\"infoPanel\");function closeCtrlModal(){ctrlModal.style.display=\"none\"}</script>";