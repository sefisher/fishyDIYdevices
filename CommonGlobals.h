//-------------------------------------------------------------------------------------------
//----------------- COMMS PARAMETERS---------------------------------------------------------
//-------------------------------------------------------------------------------------------
//define the maximum length for the text used as commands (for serial comms or in URLs as GETs)
#define MAXCMDSZ 300  

//make the webserver and web updater
AsyncWebServer httpServer(80);						//for master node web server
//DNSServer dns;										//supports AsyncWifiManager
WiFiUDP Udp;						 				//for UDP traffic between nodes
WebSocketsServer webSocket = WebSocketsServer(81);  //for websocket comms
//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
//----------------- DEVICE STATUS STORAGE PARAMETERS-----------------------------------------
//-------------------------------------------------------------------------------------------
//Some global variables to store device state
bool deviceState = LOW; //used for state data via WiFi comms (Alexa, etc) - for any FAUXMO enabled device

unsigned long deviceResponseTime = 0; //used for tracking how long the device is being moved to determine if TIMEOUT ERROR is needed

//make a typedef struct of type fishyDevice to hold data on devices on the net; and 
//then create an array of size MAX_DEVICE to store all the stuff found on the net
//IF YOU EDIT THIS STRUCT ALSO UPDATE EEPROMdata (TO SAVE INFO) AND 
//REVIEW FUNCTIONS "UDPpollReply()" and "UDPparsePollResponse()" in UDPcomms.ino, and "getJSON()" in wifi-and-webserver.ino 
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

fishyDevice deviceArray[MAX_DEVICE]; //array of fishydevices for use in creating contorl panel webpages

//struct for storing personailty data in real time and for storing in EEPROM
//remember a character is needed for the string terminations
//SW reports needed 447 bytes; left some margin
#define EEPROMsz 450
struct EEPROMdata
{
	char initstr[13] = ""; 					//13 bytes
	char namestr[41] = "";					//41 bytes
	bool master = false;					//1 byte
	char typestr[21] = "";					//21 bytes
	char groupstr[41] = "";					//41 bytes
	char note[56] = "";						//56 bytes
	char swVer[11] = "";					//11 bytes
	int timeOut = 60;						//4 bytes
	bool deviceTimedOut = false; 			//1 byte
	char deviceCustomData[256] = "";		//256 bytes - 255 characters - format: '{name=value&name=value&name=value}' (no spaces, no "&", no "=" stored in string)
} EEPROMdata;

//Struct for WiFi setting data
struct wifiConnect{
  const char *softAP_ssid = CUSTOM_DEVICE_NAME;
  const char *softAP_password = SOFT_AP_PWD;
  /* DO NOT set wifi credentials here. 
  They are configurated at runtime and stored on EEPROM */
  char ssid[32] = "";
  char password[32] = "";
  boolean connect;
  boolean softAPmode;
  int status = WL_IDLE_STATUS;
  int connectTryCount=0;
}wifiConnect;
// apIP and netMsk are Soft AP network parameters 
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress masterIP = {0, 0, 0, 0}; //the IP address of the MASTER device

bool resetOnNextLoop = false; //used to tell the device to reset after it gets to the next main operating loop

// ----------------------------------------------------------------------------------------
// Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable
// ----------------------------------------------------------------------------------------
fauxmoESP fauxmo;

