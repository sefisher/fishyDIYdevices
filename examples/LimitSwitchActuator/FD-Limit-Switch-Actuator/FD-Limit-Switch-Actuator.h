//=============================================================================
//                  Fishy Device Limit Switch Actuator
//=============================================================================
// CUSTOM GLOBALS AND TYPES

//This says EEPROM since it is extracted from the 255 char (max) string stored in the fD.myEEPROMdata struct
//that is stored in EEPROM.  This struct is dynamic (not stored), but is encoded into the char[] then stored.
struct EEPROMdeviceData
{
    bool swapLimSW = false;        //1 byte     //unique to Limit-SW-Actuator
    bool openIsCCW = true;         //1 byte     //unique to Limit-SW-Actuator
    bool motorPosAtCCWset = false; //1 byte	    //unique to Limit-SW-Actuator
    bool motorPosAtCWset = false;  //1 byte     //unique to Limit-SW-Actuator
    int motorPos = 0;              //4 bytes    //unique to Limit-SW-Actuator
    int range = 0;                 //4 bytes    //unique to Limit-SW-Actuator
} EEPROMdeviceData;

int targetPos = -1; //meaning no target motor position
enum trueState      //enum used to define stages of both static and transient status - tracked in the device only
{
    movingCW,
    fullCW,
    movingCCW,
    fullCCW,
    man_idle,
    unknown
};
static const char *trueState_String[] = {"movingCW", "fullCW", "movingCCW", "fullCCW", "man_idle", "unknown"};
enum calStages // enum used to define steps as a device is sequenced through stages of an auto-calibration
{
    doneCal,
    movingCCWCal,
    movingCWCal
};
static const char *calStages_String[] = {"doneCal", "movingCCWCal", "movingCWCal"};
calStages deviceCalStage = doneCal;  //used to sequence the device through stages of auto-cal
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state
//=============================================================================

//=============================================================================
/*  DECLARATIONS -->

CUSTOM DEVICE FUNCTIONS - EXTERNAL - The following 
COMMON EXTERNAL functions are required for all fishyDevice objects are DECLARED in
fishyDevice.h but must be DEFINED in this file using the fishyDevice namespace (fishyDevice::).

void operateDevice();
void deviceSetup(); 
void executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote);
void executeState(bool state, unsigned char, context int); 
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

void executeGoto(String cmd);
void executeStop();
trueState moveCCW();
trueState moveCW();
int whichWayGoto(int in);
trueState idleActuator(trueState idleState);
trueState openPercentActuator(int percent);
//=============================================================================

//=============================================================================

//Stepper Motor Full Swing Settings----------------------------------------------
// This sets the full stroke first "guess" for the range of motion between physical
// limit switches.  For the 90degree swing of the vent damper the 28BYJ-48 took about
// 3250 steps.  Setting it a little larger to ensure the limit switch is reached on
// boot up when it calibrates.  Future motion will be software limited.  This is defined
// to prevent a broken switch from running the motor continuously and causing damage.
#define FULL_SWING 10000
#define MAX_SPEED 1200
#define START_SPEED 600
#define ACCELERATION 200

//Pin and Comm Rate Definitions - These are for nodeMCU-------------------------
// Motor pin definitions
#define motorPin1 5 // D1=GPIO5 for IN1 on the ULN2003 driver 1
#define motorPin2 4 // D2=GPIO4 for IN2 on the ULN2003 driver 1
#define motorPin3 0 // D3=GPIO0 for IN3 on the ULN2003 driver 1
#define motorPin4 2 // D4=GPIO2 for IN4 on the ULN2003 driver 1

// Switch pin definitions
#define SWpinLimitCW 14  // D5=GPIO14 for full CW limit switch
#define SWpinLimitCCW 12 // D6=GPIO12 for full CCW limit switch

//------------------------------------------------------------------

#define HALFSTEP 8

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);


//=============================================================================================================================
//CTRL SITE PARTS - Used to serve up each device's controls
//WEBCTRLSTR is common to all devices and is REQUIRED
const char WEBCTRLSTR[] PROGMEM = "<!doctypehtml><title>fishDIY Limit-SW-Actuator</title><meta content=\"initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css rel=stylesheet id=styles><link href=styles.css rel=stylesheet><script src=CommonWebFunctions.js></script><div class=main id=myBody><script>function dealWithMessagePayload(e){var t=getMsg(e);console.log(t),setOtherTxt(t);var n=getNodeJSONtext(e);console.log(n),processJSON(n)}function setMainTxt(e){document.getElementById(\"mainTxt\").innerHTML=e+\"%\"}function setOtherTxt(e){document.getElementById(\"otherTxt\").innerHTML=e}function syncPO(){var e=document.getElementById(\"pctOpenSld\");document.getElementById(\"pobtntxt\").innerHTML=e.value+\"%\"}function openIsCCWUpd(){var e=document.getElementById(\"swLab\");1==document.getElementById(\"swChck\").checked?e.innerHTML=\"Open CCW\":e.innerHTML=\"Open CW\"}function swSwapUpd(){var e=document.getElementById(\"swSwapLab\");1==document.getElementById(\"swSwapChck\").checked?e.innerHTML=\"Swapped Lim Sw\":e.innerHTML=\"Normal Lim Sw\"}function sendCmd(e){if(1!=websock.readyState)return alert(\"This control panel is no longer connected to the device.  Please close this window and reopen the control panel.\"),0;var t=\"\";if(\"O\"==e)t=\"open\";else if(\"C\"==e)t=\"close\";else if(\"S\"==e)t=\"stop\";else if(\"cal\"==e)t=\"calibrate\";else if(\"reboot\"==e)t=\"reset\";else if(\"rstWifi\"==e)t=\"reset_wifi\";else if(\"G\"==e){var n;for(t=\"goto\",n=document.getElementById(\"pctOpenSld\").value;n.length<3;)n=\"0\"+n;t+=n}else if(\"config\"==e){var d=\"false\",o=\"false\",c=\"false\";document.getElementById(\"swMstrChck\").checked&&(d=\"true\"),document.getElementById(\"swChck\").checked&&(o=\"true\"),document.getElementById(\"swSwapChck\").checked&&(c=\"true\"),t=\"config\",t+=\";openIsCCW=\"+o,t+=\";isMaster=\"+d,t+=\";devName=\"+(\"\"==document.getElementById(\"devName\").value?\" \":document.getElementById(\"devName\").value),t+=\";note=\"+(\"\"==document.getElementById(\"devNote\").value?\" \":document.getElementById(\"devNote\").value),t+=\";swapLimSW=\"+c,t+=\";timeOut=\"+(\"\"==document.getElementById(\"timeOut\").value?\"60\":document.getElementById(\"timeOut\").value)}document.body.style.backgroundColor=\"var(--bg-col-ack)\",websock.send(t)}var infoText=\"...Loading...\";function doStuffWithJSON(e){var t=\"\",n=\"\",o=!1,c=\"\";d=e[0],returnColor(),\"true\"==d.isMaster?(t=\" (MASTER)\",document.getElementById(\"swMstrChck\").checked=!0):(t=\"\",document.getElementById(\"swMstrChck\").checked=!1),\"true\"==d.openIsCCW?document.getElementById(\"swChck\").checked=!0:(chckTxt=\"\",document.getElementById(\"swChck\").checked=!1),\"true\"==d.swapLimSW?document.getElementById(\"swSwapChck\").checked=!0:document.getElementById(\"swSwapChck\").checked=!1,n=\"true\"==d.motorPosAtCWset&&\"true\"==d.motorPosAtCCWset?\"\":(o=!0,c=\"UNCALIBRATED\\n\",\"**HARDWARE LIMITS ARE UNCALIBRATED**\\n(AUTOCAL IS RECOMMENDED)\\n\"),\"true\"==d.deviceTimedOut&&(o=!0,c+=\"ERROR - DEVICE TIMED OUT\\n\"),openIsCCWUpd(),swMstrUpd(),swSwapUpd(),infoText=\"DEVICE TYPE: \"+d.devType+\"<br>\\nNAME: \"+d.deviceName+\"<br>\\nIP ADDRESS: \"+d.ip+t+\"<hr>\\nMOTOR POSITION: \"+d.motorPos+\"<br>\\nMOTOR HARDWARE LIMITS: CCW=0 CW= \"+d.range+\"<hr>\\nSOFTWARE VERSION: \"+d.swVer+\"<br>\\nINIT STR: \"+d.initStamp+\"\\n\\n\\n<hr>\"+n+\"<br>\\n\"+c,document.getElementById(\"info-icon\").className=o?\"info-icon-blink\":\"info-icon\",setMainTxt(posPct(d.motorPos,d.range,d.openIsCCW)),document.getElementById(\"devIP\").value=d.ip+t,document.getElementById(\"devNote\").value=d.note,document.getElementById(\"timeOut\").value=d.timeOut,document.getElementById(\"devName\").value=d.deviceName,document.getElementById(\"devType\").value=d.devType}function posPct(e,t,n){if(0==t)return 0;var d=Math.round(e/t*100);return\"true\"==n&&(d=100-d),d}window.onload=start()</script></div><div class=flex-grid-outer id=flex-container><div class=fishyHdr id=deviceStatus><table width=100%><tr><td><div class=info-icon id=info-icon onclick=showDetails()>&#9432;</div><td><div class=flex-grid style=padding:6px><table><tr><td style=font-size:12px>Position:<tr><td id=mainTxt>100%</table><span class=otherTxt id=otherTxt></span></div></table></div><div><div class=buttonSet><div class=flex-grid><div class=sliderDiv><input id=pctOpenSld class=slider value=50 type=range max=100 min=0 oninput=syncPO()></div><div class=button onclick='sendCmd(\"G\")'><table width=100%><tr><td class=lilButtonTxt>GOTO<tr><td class=buttonTxt><span id=pobtntxt>50%</span></table></div></div><div class=flex-grid><div class=button onclick='sendCmd(\"O\")'><span class=buttonTxt>OPEN</span></div><div class=button onclick='sendCmd(\"S\")'><span class=buttonTxt>STOP</span></div><div class=button onclick='sendCmd(\"C\")'><span class=buttonTxt>CLOSE</span></div></div></div><div class=swPanel><div class=flex-grid><div class=swRow><label class=sw id=sw><input id=swChck type=checkbox onchange=openIsCCWUpd()><span class=sldSw></span></label><span class=swLab id=swLab>Open CW</span></div><div class=swRow><label class=sw id=swMstr><input id=swMstrChck type=checkbox onchange=swMstrUpd() checked><span class=sldSw></span></label><span class=swLab id=swMstrLab>Master Node</span></div><div class=swRow><label class=sw id=swSwap><input id=swSwapChck type=checkbox onchange=swSwapUpd() checked><span class=sldSw></span></label><span class=swLab id=swSwapLab>Swapped Lim SW</span></div></div></div></div><div class=configPanel><label class=cfgInpLab for=devIP>IP Address:<input id=devIP class=cfgInp value=...Loading... maxlength=40 disabled></label><br><label class=cfgInpLab for=devName>Name:<input id=devName class=cfgInp value=...Loading... maxlength=40 onkeypress=\"return blockSpecialChar(event)\"></label><br><label class=cfgInpLab for=devType>Type:<input id=devType class=cfgInp value=...Loading... maxlength=20 disabled></label><br><label class=cfgInpLab for=devNote>Note:<input id=devNote class=cfgInp value=...Loading... maxlength=55 onkeypress=\"return blockSpecialChar(event)\"></label><br><label class=cfgInpLab for=timeOut>Time-Out:<input id=timeOut class=cfgInpNum value=00 maxlength=5 onkeypress=\"return numberCharOnly(event)\"title=\"Enter a time (in seconds) to wait for the actuator to respond before stopped and displaying an error message (assuming the actuator got stuck).\"></label></div><div class=fishyFtrSw><div class=flex-grid><input id=updCfgBtn class=cfgbuttonY value=\"UPDATE SETTINGS\"title=\"This will save new settings in the device. New settings should be displayed on next refresh.  If trying to adjust mulitple settings it is recommended that you temporarily turn off Auto-refresh.\"onclick='sendCmd(\"config\")'type=button> <input id=updCfgBtn class=cfgbuttonY value=\"REBOOT DEVICE\"title=\"This will reboot the device.\"onclick='sendCmd(\"reboot\")'type=button></div><div class=flex-grid><input id=updCfgBtn class=cfgbuttonR value=\"AUTO-CAL. HW LIM\"title=\"WARNING:Auto-Cal will attempt to cycle your actuator full open and shut to determine  hardware limit switch positions. Ensure that range is possible to prevent damage.\"onclick='sendCmd(\"cal\")'type=button> <input id=rstWifiBtn class=cfgbuttonR value=\"RESET WIFI\"title=\"WARNING:Reset Wifi will attempt to delete your network SSID and passwordand put the device into wifi-server mode to learn new wifi SSID and password. To teach a reset device a new wifi network go to the device IP address using a mobile phone or computer.\"onclick='sendCmd(\"rstWifi\")'type=button></div></div></div><div class=infoPanel id=infoPanel><div class=infoDiv id=infoDiv></div><input class=infoOKbtn value=OK type=button onclick=closeCtrlModal()></div><script>var ctrlModal=document.getElementById(\"infoPanel\");function closeCtrlModal(){ctrlModal.style.display=\"none\"}</script>";