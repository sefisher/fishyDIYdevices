//-----------------------------------------------------------------------------
//--------------------------USER DEFINED VARIABLES-----------------------------
//-------------------(See fishyDevices.h for DEBUG switches)-------------------
//-----------(See FD-Limit-Switch-Actuator.h for device type specific settings)----------
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
extern const char SW_VER[] = "20190328-1";

//------------------------DEVICE TYPE SETTINGS------------------------------//
//ENTER THIS DEVICE'S TYPE USING ONLY LETTERS AND NUMBERS (NO SPACES)
//TYPE MUST BE EXACTLY SAME AS TYPE LISTED IN CommonDeviceTranslator.ino
//LIMITED to 20 alpanumeric characters
extern const char CUSTOM_DEVICE_TYPE[] = "Limit-SW-Actuator";
//extern const char CUSTOM_DEVICE_TYPE[] = "RGBLED";

//---------------------Limit-SW-Actuator UNIQUE SETTINGS---------------------//
//---------------------(IGNORED BY OTHER TYPES)------------------------------//

//FIGURE OUT WHAT DIRECTION YOU WANT TO PICK AS OPEN or CLOSE FOR YOUR DEVICE.
//(This can be changed later through the web interface).
extern const bool OPEN_IS_CCW = false;

//Software fix in case you miswired and swapped the hardware limit switches.
//(This can be changed later through the web interface).
extern const bool SWAP_LIM_SW = false;

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


