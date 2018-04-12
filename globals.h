//-------------------------------------------------------------------------------------------
//----------------- COMMS PARAMETERS---------------------------------------------------------
//-------------------------------------------------------------------------------------------
//define the maximum length for the text used as commands (for serial comms or in URLs as GETs)
#define MAXCMDSZ 300  

//make the webserver and web updater
ESP8266WebServer httpServer(80);	 //for master node web server
ESP8266HTTPUpdateServer httpUpdater; //for processing software updates
WiFiUDP Udp;						 //for UDP traffic between nodes
WiFiManager WiFiManager;			 //for managing Wifi

//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
//----------------- DEVICE STATUS STORAGE PARAMETERS-----------------------------------------
//-------------------------------------------------------------------------------------------
//Some global variables to store device state
bool deviceState = LOW; //used for state data via WiFi comms (Alexa, etc)

float currentSpeed = stepper1.speed();

enum trueState //enum used to define stages of both static and transient status - tracked in the device only
{
	opening,
	opened,
	closing,
	closed,
	man_idle,
	man_CCW,
	man_CW,
	unknown
}; 
static const char *trueState_String[] = {"opening", "opened", "closing", "closed", "man_idle", "man_CCW", "man_CW", "unknown"};
enum calStages // enum used to define steps as a device is sequenced through stages of an auto-calibration
{
	doneCal,
	openingCal,
	closingCal
};
static const char *calStages_String[] = {"doneCal", "openingCal", "closingCal"};
calStages deviceCalStage = doneCal; //used to sequence the device through stages of auto-cal
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state

int currentPos;
enum manCommandState
{
	none,
	CW,
	CCW,
	SEL
}; //used for tracking the commanded state with manual controls
static const char *manCommandState_String[] = {"none", "CW", "CCW", "SEL"};

int manSelCnt = 0;					   //used to track how many times the select switch is pressed sequentially
manCommandState lastManCommand = none; //used to track last manual command (control box) to determine what a "select" command does

//make a typedef struct of type fishyDevice to hold data on devices on the net; and 
//then create an array of size MAX_DEVICE to store all the stuff found on the net
typedef struct fishyDevice
{
	IPAddress ip;
	String name = "Default";
	int port = 8266;
	bool isCalibrated = false;
	int motorPosAtCCW = -FULL_SWING + 3;
	int motorPosAtCW = FULL_SWING - 3;
	int motorPos = 0;
	//starting positions (guesses) for the limit switches.  FULL_SWING should be set to be a little more than full travel.
	int motorPosAtFullCCW = -FULL_SWING; 
	int motorPosAtFullCW = FULL_SWING;
	bool openIsCCW = true;
	bool isMaster;
	bool dead = true;
	String group = "";
	String note = "";
	String swVer = "";
	String devType = "";
	bool motorPosAtCCWset = false;
	bool motorPosAtCWset = false;
} fishyDevice;

//210 byte struct for storing personailty data in real time and for storing in EEPROM
//remember a character is needed for the string terminations
#define EEPROMsz 210
//motor data starts at addr 188 and is 22 bytes long (when just updating that data)
#define EEPROMmotDataAddr 188
#define EEPROMmotDataSz 22
struct EEPROMdata
{
	char initstr[13] = ""; 					//13 bytes
	char namestr[41] = "";					//41 bytes
	bool master = 0;						//1 byte
	char typestr[21] = "";					//21 bytes
	char groupstr[41] = "";					//41 bytes
	char note[56] = "";						//56 bytes
	char openIsCCW[4] = "";					//4 bytes
	char swVer[11] = "";					//11 bytes
	//used to store "soft" limits set by users to constrain open-close range
	int motorPosAtCCW = -FULL_SWING + 3; 	//4 bytes
	int motorPosAtCW = FULL_SWING - 3;		//4 bytes
	//used to store actual limit SW position for reference (if obtainable)
	int motorPosAtFullCCW = -FULL_SWING; 	//4 bytes
	int motorPosAtFullCW = FULL_SWING;		//4 bytes
	bool motorPosAtCCWset = false;			//1 byte
	bool motorPosAtCWset = false;			//1 byte
	int motorPos = 0; 						//4 bytes
} EEPROMdata;

fishyDevice deviceArray[MAX_DEVICE];
IPAddress masterIP = {0, 0, 0, 0};
bool resetOnNextLoop = false; //used to tell the device to reset after it gets to the next main operating loop

// ----------------------------------------------------------------------------------------
// Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable
// ----------------------------------------------------------------------------------------
fauxmoESP fauxmo;