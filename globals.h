//-------------------------------------------------------------------------------------------
//----------------- COMMS PARAMETERS---------------------------------------------------------
//-------------------------------------------------------------------------------------------
//define the maximum length for the text used as commands (for serial comms or in URLs as GETs)
#define MAXCMDSZ 300  

//make the webserver and web updater
AsyncWebServer httpServer(80);						//for master node web server
DNSServer dns;										//supports AsyncWifiManager
WiFiUDP Udp;						 				//for UDP traffic between nodes
WebSocketsServer webSocket = WebSocketsServer(81);  //for websocket comms

//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
//----------------- DEVICE STATUS STORAGE PARAMETERS-----------------------------------------
//-------------------------------------------------------------------------------------------
//Some global variables to store device state

//TODO - DELETE? - this seems to be referenced in some status outputs but never used
bool deviceState = LOW; //used for state data via WiFi comms (Alexa, etc) - for any FAUXMO enabled device

//float currentSpeed = stepper1.speed(); //CUSTOM?

unsigned long deviceResponseTime = 0; //used for tracking how long the device is being moved to determine if TIMEOUT ERROR is needed

typedef struct fishyDevice
{
	IPAddress ip;
	String name = "";
	String typestr = "";				
	String groupstr = "";				
	String statusstr = "";
	bool inError = false; //captures timeout, not being calibrated, and any device specifc errors
	bool isMaster;
	bool dead = true;
	unsigned long timeStamp=0;	//used to track when device updates were made last to cull dead nodes

} fishyDevice;

//struct for storing personailty data in real time and for storing in EEPROM
//remember a character is needed for the string terminations
//SW reports needed 204 bytes; left some margin
#define EEPROMsz 204

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

fishyDevice deviceArray[MAX_DEVICE]; //array of fishydevices for use in creating contorl panel webpages
IPAddress masterIP = {0, 0, 0, 0}; //the IP address of the MASTER device - TODO - consider for deletion
bool resetOnNextLoop = false; //used to tell the device to reset after it gets to the next main operating loop

// ----------------------------------------------------------------------------------------
// Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable
// ----------------------------------------------------------------------------------------
fauxmoESP fauxmo;



//=========================================================================================
//TODO - CUSTOM - move to 2-state-actuator (this section)
//=========================================================================================
//TODO - CUSTOM - move to 2-state-actuator
int targetPos = -1; //meaning no target
//TODO - CUSTOM - move to 2-state-actuator
enum trueState //enum used to define stages of both static and transient status - tracked in the device only
{
	opening,
	opened,
	closing,
	closed,
	man_idle,
	unknown
}; 
//TODO - CUSTOM - move to 2-state-actuator
static const char *trueState_String[] = {"opening", "opened", "closing", "closed", "man_idle", "unknown"};
//TODO - CUSTOM - move to 2-state-actuator
enum calStages // enum used to define steps as a device is sequenced through stages of an auto-calibration
{
	doneCal,
	openingCal,
	closingCal
};
//TODO - CUSTOM - move to 2-state-actuator
static const char *calStages_String[] = {"doneCal", "openingCal", "closingCal"};
//TODO - CUSTOM - move to 2-state-actuator
calStages deviceCalStage = doneCal; //used to sequence the device through stages of auto-cal
//TODO - CUSTOM - move to 2-state-actuator
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state

//make a typedef struct of type fishyDevice to hold data on devices on the net; and 
//then create an array of size MAX_DEVICE to store all the stuff found on the net
//IF YOU EDIT THIS STRUCT ALSO UPDATE EEPROMdata (TO SAVE INFO) AND 
//REVIEW FUNCTIONS "UDPpollReply()" and "UDPparsePollResponse()" in UDPcomms.ino, and "getJSON()" in wifi-and-webserver.ino 
//==============================================================