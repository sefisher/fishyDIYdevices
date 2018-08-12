//-------------------------------------------------------------------------------------------
//----------------- COMMS PARAMETERS---------------------------------------------------------
//-------------------------------------------------------------------------------------------
//define the maximum length for the text used as commands (for serial comms or in URLs as GETs)
#define MAXCMDSZ 300  

//make the webserver and web updater
ESP8266WebServer httpServer(80);	 //for master node web server
//customHTTPUpdateServer httpUpdater; //for processing software updates
WiFiUDP Udp;						 //for UDP traffic between nodes
WiFiManager WiFiManager;			 //for managing Wifi

//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
//----------------- DEVICE STATUS STORAGE PARAMETERS-----------------------------------------
//-------------------------------------------------------------------------------------------
//Some global variables to store device state
bool deviceState = LOW; //used for state data via WiFi comms (Alexa, etc)

float currentSpeed = stepper1.speed();

unsigned long motorRunTime = 0; //used for tracking how long the actuator is being moved to determine if timeout is needed

enum trueState //enum used to define stages of both static and transient status - tracked in the device only
{
	opening,
	opened,
	closing,
	closed,
	man_idle,
	unknown
}; 
static const char *trueState_String[] = {"opening", "opened", "closing", "closed", "man_idle", "unknown"};
enum calStages // enum used to define steps as a device is sequenced through stages of an auto-calibration
{
	doneCal,
	openingCal,
	closingCal
};

static const char *calStages_String[] = {"doneCal", "openingCal", "closingCal"};
calStages deviceCalStage = doneCal; //used to sequence the device through stages of auto-cal
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state

//make a typedef struct of type fishyDevice to hold data on devices on the net; and 
//then create an array of size MAX_DEVICE to store all the stuff found on the net
//IF YOU EDIT THIS STRUCT ALSO UPDATE EEPROMdata (TO SAVE INFO) AND 
//REVIEW FUNCTIONS "UDPpollReply()" and "UDPparsePollResponse()" in UDPcomms.ino, and "getJSON()" in wifi-and-webserver.ino 
typedef struct fishyDevice
{
	IPAddress ip;
	String name = "Default";
	int port = 8266;
	bool isCalibrated = false;
	int motorPos = 0;
	bool openIsCCW = true;
	bool isMaster;
	bool dead = true;
	String group = "";
	String note = "";
	String swVer = "";
	String devType = "";
	bool motorPosAtCCWset = false;
	bool motorPosAtCWset = false;
	String initStamp;
	int range;
	unsigned long timeStamp=0;	//used to track when device updates were made last to cull dead nodes
	int timeOut; //actuator timeout limit for continuing motion without reaching a stop
	bool deviceTimedOut = false; //if times out then set flag to true
	bool swapLimSW = false; //used to swap hardware limit switches is CCW and CW switches were miswired
} fishyDevice;

//struct for storing personailty data in real time and for storing in EEPROM
//remember a character is needed for the string terminations
//SW reports needed 200 bytes; left some margin
#define EEPROMsz 230

struct EEPROMdata
{
	char initstr[13] = ""; 					//13 bytes
	char namestr[41] = "";					//41 bytes
	bool master = false;					//1 byte
	char typestr[21] = "";					//21 bytes
	char groupstr[41] = "";					//41 bytes
	char note[56] = "";						//56 bytes
	bool openIsCCW = true;					//1 byte
	char swVer[11] = "";					//11 bytes
	bool motorPosAtCCWset = false;			//1 byte
	bool motorPosAtCWset = false;			//1 byte
	int motorPos = 0; 						//4 bytes
	int range = 0; 							//4 bytes
	int timeOut = 60;						//4 bytes
	bool deviceTimedOut = false; 			//1 byte
	bool swapLimSW = false;					//1 byte
} EEPROMdata;

fishyDevice deviceArray[MAX_DEVICE];
IPAddress masterIP = {0, 0, 0, 0};
int targetPos = -1; //meaning no target
bool resetOnNextLoop = false; //used to tell the device to reset after it gets to the next main operating loop

// ----------------------------------------------------------------------------------------
// Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable
// ----------------------------------------------------------------------------------------
fauxmoESP fauxmo;