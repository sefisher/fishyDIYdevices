//-----------------------------------------------------------------------------
//--------------------------USER DEFINED VARIABLES-----------------------------
//-------------------(See fishyDevices.h for DEBUG switches)-------------------
//-----------(See FD-Simple-Switch.h for device type specific settings)----------
//---In a hurry to test? Set device type and leave the rest (update via web).--
//-----------------------------------------------------------------------------

//------------------FISHYDEVICE COMMON PERSONALITY DATA------------------------
//Can be updated via web interface later. These won't be overwritten during 
//software updates unless INITIALIZE is changed (see below). 

//SET A UNIQUE NAME FOR EACH DEVICE USING ONLY LETTERS,NUMBERS, AND SPACES. 
//(This can be changed later through web interface).
//LIMITED to 40 alpanumeric characters (abcdefghijklmnopqrstuvwxyz 0123456789)
extern const char CUSTOM_DEVICE_NAME[] = "New Device";

//PICK ONLY ONE DEVICE ON THE NETWORK AS MASTER (and set this to true when you
//compile its code). IT WILL BE THE WEBSERVER AND CONTROLLER FOR ALL OTHER 
//NODES.  Set false for all the others. (This can be changed later through the
//web interface).
extern const bool MASTER_NODE = false;

//SET THE TIMEOUT (in seconds) FOR STOPPING THE ACTUATOR IF MOTION CONTINUES 
//WITHOUT REACHING A HARDWARE LIMIT. (This can be changed later through the
//web interface).
extern const int DEVICE_TIMEOUT = 60;

//IF DESIRED, ADD A NOTE ABOUT THIS DEVICE. The note is displayed on the control 
//panel for the device. (This can be changed later through the web interface).
//LIMITED to 55 alpanumeric characters
extern const char CUSTOM_NOTE[] = "";

//--------------------------SOFTWARE SETTINGS--------------------------------//
//SW_VER is for keeping track of SW version it is displayed but not otherwise used.
//You can update code - update this number - compile - and then load SW onto all the 
//devices without modifying the other personality settings as long as INITIALIZE is 
//the same on the devices (see below).
//LIMITED to 10 alpanumeric characters
extern const char SW_VER[] = "20191110-1";

//----------------------OTHER SETTINGS BASED ON HARWARE SETUP---------------//
//This allows disabling or enabling Serial Input (RX); generally set to FALSE to allow using the associated GPIO (e.g., GPIO3) as a normal pin
extern const bool USE_SERIAL_INPUT = false;
//This allows disabling or enabling blinking functions using either the builtin LED of defined LED
extern const bool DO_BLINKING = true;
//Set BLINK_LED to LED_BUILTIN by default
extern const int BLINK_LED = 14;


//------------SIMPLE-SWITCH TYPE SPECIFIC DEVICE SETTINGS------------------------//
//the number of distinct switches (relays) you can add (can be set from 1 to 4)
#define NUM_SWITCHES 4
//names are LIMITED to 40 alpanumeric characters (abcdefghijklmnopqrstuvwxyz 0123456789)
//The First switch name i set to CUSTOM_DEVICE_NAME
//Uncomment and define all 3 of these even if you only have 1,2, or 3 switches to 
//allow future changes and prevent software errors on initialization
#define DEVICE_2_NAME "Switch 2"
#define DEVICE_3_NAME "Switch 3"
#define DEVICE_4_NAME "Switch 4"

//------------OTHER NON-TYPE SPECIFIC DEVICE SETTINGS------------------------//
//--------------------(INFREQUENTLY CHANGED)---------------------------------//

//These should generally be the same on all your devices since they are overwritten 
//with every softare update. These are hardcoded (can't be changed other than 
//recompiling and loading new software).        

//password for connecting to the device as an Access Point when it can't 
//connect to your WIFI network (ssid will be CUSTOM_DEVICE_NAME)
extern const char SOFT_AP_PWD[] = "123456789"; 

//EEPROM MEMORY MANAGEMENT SETTING

// INITIALIZE is used to indicate when all device personality settings should be overwritten with
// those provided in the compiled code. It basically defines a specific EEPROM storage structure. 
// This is useful if you change EEPROM storage structure in a software update.  Usually it should 
// remain fixed and be the same for all devices on your network so that other software updates don't 
// affect the personality and status data stored in each device.

// On software load the device will look to see if this INITIALIZE value and that stored 
// in its EEPROM are different; if so the rest of the settings defined here will be will be loaded 
// and other stored status information will be reset (e.g., motor position, etc).  
// If INITIALIZE values are the same the new software (and version number above) will be used 
// but the device will keep the old EEPROM settings (ignoring all the stuff below) and retain any 
// stored device position and calibration info.  

// Leaving INITIALIZE the same on all devices allows a single software upload to be 
// used for multiple devices without overwritting their "personality settings" and 
// wiping out stored motor info. 

// ***IF YOU CHANGE THIS YOU NEED TO RECOMPILE FOR EACH DEVICE TO AVOID OVERWRITING 
// THEIR COMPILED DEVICE_TYPE AND OTHER DATA IN FUTURE***

//String format -> "YYYYMMDDHHmm" (where HHmm is 24 hr time).
extern const char INITIALIZE[] = "201903082305";

//-----------------------------------------------------------------------------------


