//-----------------------------------------------------------------------------------
//--------------------------USER DEFINED VARIABLES-----------------------------------
//-----------------------------------------------------------------------------------

/*TODO implement this (store these settings in EEPROM and only used these settings
if INITIALIZE is true. -->  This should only be set true when you want to initialize the 
identity and initial settings of the device (e.g., first time you load software).  
Once done, setting this to false will allow software updates to be pushed to all 
devices through the httpServer without overwriting the name, master, and other basic
configuration data. */
#define INITIALIZE false

//PICK A UNIQUE NAME FOR EACH DEVICE USING ONLY LETTERS AND NUMBERS
// #define CUSTOM_DEVICE_NAME "Vent Damper 1"
#define CUSTOM_DEVICE_NAME "Vent Damper 2"
// #define CUSTOM_DEVICE_NAME "Vent Damper 3"

//PICK ONLY ONE DEVICE ON THE NETWORK AS MASTER (and set this to true when you compile its code).  
//IT WILL BE THE WEBSERVER AND CONTROLLER FOR ALL OTHER NODES.  Set false for all the others.
#define MASTER_NODE true
// #define MASTER_NODE false

//FIGURE OUT WHAT DIRECTION YOU WANT TO PICK AS OPEN or CLOSE FOR YOUR DEVICE.  
//TODO - use this parameter
#define OPEN_IS_CCW_OR_CW "CCW"
//#define OPEN_IS_CCW_OR_CW "CW"

//----------------------------------------------------------------------------------
//Board communication rate
#define SERIAL_BAUDRATE 115200
#define UDP_LOCAL_PORT 8266

//For the MASTER NODE (webserver node) this sets the number of devices it can manage on the net
#define MAX_DEVICE 48

//Test switches for Serial output text (set to false to disable debug messages) and WiFi Functions
#define DEBUG_MESSAGES true
#define UDP_PARSE_MESSAGES true
#define DEBUG_HEAP_MESSAGE false
#define DEBUG_WiFiOFF false //turn to true to test without wifi
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------