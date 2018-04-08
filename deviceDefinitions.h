//-----------------------------------------------------------------------------------
//--------------------------USER DEFINED VARIABLES-----------------------------------
//-----------------------------------------------------------------------------------

//--------------------------PERSONALITY SETTINGS-----------------------------------//
//Put in a time/date string to define the personality variables. On load the device will look to
//see if the settings stored in EEPROM are different; if so they will be loaded.  If not
//they will use the new software but keep the old EEPROM settings.  This allows a single 
//software upload to be used for multiple devices without overwritting their "personality settings".
//String format -> "YYYYMMDDHHmm" (where HHmm is 24 hr time). The other device settings 
//are used every time software is loaded
#define INITIALIZE "201804072033"

//this is for keeping track of SW version
//it is displayed but not otherwise used
//LIMITED to 10 alpanumeric characters
#define SW_VER "20180407-5"

//PICK A UNIQUE NAME FOR EACH DEVICE USING ONLY LETTERS AND NUMBERS
// LIMITED to 40 alpanumeric characters (abcdefghijklmnopqrstuvwxyz0123456789)
// #define CUSTOM_DEVICE_NAME "Vent Damper 1"
#define CUSTOM_DEVICE_NAME "Vent Damper 2"
// #define CUSTOM_DEVICE_NAME "Vent Damper 3"

//PICK ONLY ONE DEVICE ON THE NETWORK AS MASTER (and set this to true when you compile its code).  
//IT WILL BE THE WEBSERVER AND CONTROLLER FOR ALL OTHER NODES.  Set false for all the others.
#define MASTER_NODE true
// #define MASTER_NODE false

//FIGURE OUT WHAT DIRECTION YOU WANT TO PICK AS OPEN or CLOSE FOR YOUR DEVICE.  
//TODO - use this parameter correctly
#define OPEN_IS_CCW_OR_CW "CCW"
// #define OPEN_IS_CCW_OR_CW "CW"

//IF DESIRED, SPECIFY THE DEVICE TYPE
//LIMITED to 20 alpanumeric characters 
//The note is displayed on the control panel for the device.
//TODO - use this option
#define CUSTOM_DEVICE_TYPE ""

//IF DESIRED, PICK A GROUP NAME FOR EACH DEVICE USING ONLY LETTERS/NUMBERS
//LIMITED to 40 alpanumeric characters 
//This creates a group of devices that can be controlled together.
//TODO - use this option
#define CUSTOM_GROUP_NAME ""

//IF DESIRED, ADD A NOTE ABOUT THIS DEVICE
//LIMITED to 55 alpanumeric characters 
//The note is displayed on the control panel for the device.
#define CUSTOM_NOTE ""

//----------------------------OTHER DEVICES SETTINGS-------------------------------//
//Board communication rate
#define SERIAL_BAUDRATE 115200
#define UDP_LOCAL_PORT 8266

//For the MASTER NODE (webserver node) this sets the number of devices it can manage on the net
#define MAX_DEVICE 24

//Test switches for Serial output text (set to false to disable debug messages) and WiFi Functions
#define DEBUG_MESSAGES true
#define UDP_PARSE_MESSAGES false
#define DEBUG_HEAP_MESSAGE false
#define DEBUG_WiFiOFF false //turn to true to test without wifi
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------