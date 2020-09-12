//TODO - go through and update the templates from recent changes

//=============================================================================
//                  Fishy Device [[TEMPLATE]]
//=============================================================================
// CUSTOM GLOBALS AND TYPES
//------------------------DEVICE TYPE SETTINGS------------------------------//
//ENTER THIS DEVICE'S TYPE USING ONLY LETTERS AND NUMBERS (NO SPACES)
//LIMITED to 20 alpanumeric characters
extern const char CUSTOM_DEVICE_TYPE[] = "TEMPLATE";  //TODO 1 - rename this to your device name

//This says EEPROM since it is extracted from the 255 char (max) string stored in the fD.myEEPROMdata struct
//that is stored in EEPROM.  This struct is dynamic (not stored), but is encoded into the char[] then stored.
struct EEPROMdeviceData
{
    //TODO 2 - Put variables you need to store for your device (they'll be encoded into a string and saved in EEPROM) 
} EEPROMdeviceData;

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

//TODO 3 - DECLARE any new device functions you create for your device here
// (they should be DEFINED in the .ino file)

//=============================================================================

//TODO 4 - this is where you should put any #define statements or other constants 
// needed within your code to include unique pin assignments for your new device


//==============================================================================
//CTRL SITE PARTS - Used to serve up each device's controls
//WEBCTRLSTR is common to all devices and is REQUIRED

//TODO 5 - create your web control page by customizing from one of the
// example ControlTemplate.html files and then use the instructions in that file
// to minify the html and make it into a string to be pasted below

const char WEBCTRLSTR[] PROGMEM = "<!doctypehtml><title>fishDIY [[TEMPLATE]]</title><meta content=\"initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css rel=stylesheet id=styles><link href=styles.css rel=stylesheet><script src=CommonWebFunctions.js></script>";