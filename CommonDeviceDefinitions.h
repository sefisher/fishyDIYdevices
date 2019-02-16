//-----------------------------------------------------------------------------------
//--------------------------USER DEFINED VARIABLES-----------------------------------
//-----------------------------------------------------------------------------------

//This is for keeping track of SW version it is displayed but not otherwise used.
//You can update code - update this number - compile - and then load SW onto all the devices without
//modifying the personality settings as long as INITIALIZE is the same on the devices (see below)
//LIMITED to 10 alpanumeric characters
#define SW_VER "20181202-1"

//--------------------------PERSONALITY SETTINGS-----------------------------------//
//Put in a time/date string to define the personality variables. 

/*
On load the device will look to see if this setting and that stored in EEPROM are 
different; if so the rest of the settings will be will be loaded.  If not the new 
software (and version number above) will be used but the device will keep the old 
EEPROM settings (ignoring all the stuff below) and any stored motor position 
and calibration info.  

Leaving INITIALIZE the same on all devices allows a single software upload to be 
used for multiple devices without overwritting their "personality settings" and 
wiping out stored motor info. 

***IF YOU CHANGE THIS YOU NEED TO RECOMPILE FOR EACH DEVICE TO AVOID OVERWRITING 
THEIR COMPILED DEVICE_TYPE AND OTHER DATA***
*/
//String format -> "YYYYMMDDHHmm" (where HHmm is 24 hr time). 
#define INITIALIZE "201812120600"

//PICK A UNIQUE NAME FOR EACH DEVICE USING ONLY LETTERS AND NUMBERS
// LIMITED to 40 alpanumeric characters (abcdefghijklmnopqrstuvwxyz0123456789)
#define CUSTOM_DEVICE_NAME "Floor 3 Vent"
// #define CUSTOM_DEVICE_NAME "Floor 2 Vent"
// #define CUSTOM_DEVICE_NAME "Floor 1 Vent"
//#define CUSTOM_DEVICE_NAME "RGB LED Test"
// #define CUSTOM_DEVICE_NAME "Test Device 23"

//ENTER THIS DEVICE'S TYPE USING ONLY LETTERS AND NUMBERS (NO SPACES)
//TYPE MUST BE EXACTLY SAME AS TYPE LISTED IN CommonDeviceTranslator.ino
//LIMITED to 20 alpanumeric characters 
#define CUSTOM_DEVICE_TYPE "2-State-Actuator"
//#define CUSTOM_DEVICE_TYPE "RGBLED"

//PICK ONLY ONE DEVICE ON THE NETWORK AS MASTER (and set this to true when you compile its code).  
//IT WILL BE THE WEBSERVER AND CONTROLLER FOR ALL OTHER NODES.  Set false for all the others.
#define MASTER_NODE true
// #define MASTER_NODE false

//SET THE TIMEOUT (in seconds) FOR STOPPING THE ACTUATOR IF MOTION CONTINUES WITHOUT REACHING A HARDWARE LIMIT
#define DEVICE_TIMEOUT 60

//IF DESIRED, PICK A GROUP NAME FOR EACH DEVICE USING ONLY LETTERS/NUMBERS
//LIMITED to 40 alpanumeric characters 
//This creates a group of devices that can be controlled together.
//TODO - delete this option - making new Group operation features
#define CUSTOM_GROUP_NAME ""

//IF DESIRED, ADD A NOTE ABOUT THIS DEVICE
//LIMITED to 55 alpanumeric characters 
//The note is displayed on the control panel for the device.
#define CUSTOM_NOTE ""

//TESTING THESE - ADDING ALTERNATE DEVICE NAMES FOR ALTERNATE VOICE COMMAND ACTIONS
//IF defined THESE WILL BE NEW FAUXMO DEVICES.  Currently upto 3.
//#define ALT1_DEVICE_NAME "Face Kitchen"
//#define ALT2_DEVICE_NAME "Face Livingroom"
//#define ALT3_DEVICE_NAME "Face Livingroom"

//----------------------------2-state-actuator UNIQUE SETTINGS---------------------//
//----------------------------IGNORED BY OTHER TYPES--------------------------------//

//TODO - move these settings to 2-state-actuator files

//FIGURE OUT WHAT DIRECTION YOU WANT TO PICK AS OPEN or CLOSE FOR YOUR DEVICE.  
#define OPEN_IS_CCW true
//#define OPEN_IS_CCW false

//Software fix in case you miswired and swapped the hardware limit switches.  
// #define SWAP_LIM_SW false 
#define SWAP_LIM_SW true //Floor 3 Vent, Floor 1 Vent

//----------------------------OTHER DEVICES SETTINGS-------------------------------//
#define SOFT_AP_PWD "12345678" //This is a hardcoded (can't be changed other than recompiling and loading software compile) password for connecting to the device as an Access Point when it can't connect to your WiFi
#define ESP8266 //tell libraries this is an ESP8266 (vice ESP32)


#define SW_UPDATE_DELAY 10 //the number of seconds to wait for a reboot following a software update - increase if you see "page not found errors" after an update

//SET THIS TO TRUE IF YOU WANT A SIMPLE "TURN ON, TURN OFF" COMMAND CAPABILITY FOR 
//CONTROLLING THE DEVICE THROUGH AMAZON ECHO (ALEXA) OR A SMART HOME HUB (LIKE SMARTTHINGS).
//THE DEVICE WOULD THEN SHOW UP ON YOUR WIFI NETWORK AS A "WEMO PLUG" OR SIMILAR.
//"TURN ON" = FULLY OPEN; "TURN OFF" = FULLY CLOSE.  
//THIS IS NOT CONFIGURABLE AS A SETTING AFTER COMPILING AND UPLOADING SOFTWARE
#define FAUXMO_ENABLED true

//Board communication rate
#define SERIAL_BAUDRATE 115200
#define UDP_LOCAL_PORT 8266

//For the MASTER NODE (webserver node) this sets the number of devices it can manage on the net
#define MAX_DEVICE 24

//how many of the last commands to store for use in making group command (applies to MASTER NODE)
#define MAX_COMMANDS 5

//Test switches for Serial output text (set to false to disable debug messages) and WiFi Functions
#define DEBUG_MESSAGES true //debugging for device problems (movement, switches, etc)
#define DEBUG_UDP_MESSAGES false //debugging for network comms (MASTER - SLAVE issues with nodes on the network)
#define UDP_PARSE_MESSAGES false //debugging for parsing messages - used after you've changed the message structures
#define DEBUG_HEAP_MESSAGE false //just tracking the heap size for memory leak issues or overloaded nodeMCUs
#define DEBUG_WIFI_MESSAGES false //shows wifi connection debugging info
#define DEBUG_WiFiOFF false //turn to true to test without wifi (limited testing)
#define DEBUG_TIMING false
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------