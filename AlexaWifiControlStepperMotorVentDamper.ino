/*
You shouldn't have to edit this code in this file.  See deviceDefinitions.h
for the customization variables.

TODO - update this when the setup is on first use

This file is is setup to work with a NodeMCU 1.0 module (ESP-12E).
If you have a variant with a different PIN-arrangement you may need
to edit the pin definitions listed below.

To make this work in your home you need to:
	1) Connect to your device as an access point when it first boots and
	provide the WiFi SSID and password of you network
	(the device should come up as a WiFi network with the name you selected as a
	device on your phone's WiFi list - so goto your WiFi selection and switch to
	it to connect to the device the first time).

	2) Edit below to pick a unique name for this device.
	(This name will be used by Alexa or SmartThings).

	3) Compile and upload this file to your device.
	(if you change your WiFi password you will need to update the file
	and reload it for each device. The USB port should remain accessible
	in your fully assembled device for that purpose).


CREDITS:
Thanks for all the coders posting online to make this possible; these are a few key references:
fauxmoESP - https://bitbucket.org/xoseperez/fauxmoesp
AccelStepper - http://www.airspayce.com/mikem/arduino/AccelStepper/index.html
WiFiManager -  //https://github.com/tzapu/WiFiManager
*/

#include <Arduino.h>
#include "deviceDefinitions.h"
#include "webresources.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <fauxmoESP.h>
#include <AccelStepper.h>
#include <EEPROM.h>

//Stepper Motor Full Swing Settings----------------------------------------------
// This sets the full stroke first "guess" for the range of motion between physical
// limit switches.  For the 90degree swing of the vent damper the 28BYJ-48 took about
// 3250 steps.  Setting it a little larger to ensure the limit switch is reached on
// boot up when it calibrates.  Future motion will be software limited.  This is defined
// to prevent a broken switch from running the motor continuously and causing damage.
#define FULL_SWING 3500
#define MAX_SPEED 1000
#define START_SPEED 200
#define ACCELERATION 200

//Define manual step increment and speed
#define MANSTEP 16
#define MANSPEED 500

//Pin and Comm Rate Definitions - These are for nodeMCU-------------------------
// Motor pin definitions
#define motorPin1 5 // D1=GPIO5 for IN1 on the ULN2003 driver 1
#define motorPin2 4 // D2=GPIO4 for IN2 on the ULN2003 driver 1
#define motorPin3 0 // D3=GPIO0 for IN3 on the ULN2003 driver 1
#define motorPin4 2 // D4=GPIO2 for IN4 on the ULN2003 driver 1

// Switch pin definitions
#define SWpinLimitCW 14  // D5=GPIO14 for full close (CW) limit switch
#define SWpinLimitCCW 12 // D6=GPIO12 for full open (CCW) limit switch
#define SWpinManCW 9	 // SDD2=GPIO13 for close (CW) manual switch
#define SWpinManCCW 10   // SDD3=GPIO15 for open (CCW) manual switch
#define SWpinManSel 16   // D0=GPIO16 for select manual switch

//------------------------------------------------------------------

#define MAXCMDSZ 300  

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
#define HALFSTEP 8
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

//make the webserver and web updater
ESP8266WebServer httpServer(80);	 //for master node web server
ESP8266HTTPUpdateServer httpUpdater; //for processing software updates
WiFiUDP Udp;						 //for UDP traffic between nodes

//Some global variables to store device state
bool deviceState = LOW; //used for WiFi comms
enum trueState
{
	opening,
	opened,
	closing,
	closed,
	man_idle,
	man_CCW,
	man_CW,
	unknown
}; //used for internal state tracking
static const char *trueState_String[] = {"opening", "opened", "closing", "closed", "man_idle", "man_CCW", "man_CW", "unknown"};
enum calStages
{
	doneCal,
	openingCal,
	closingCal
};
static const char *calStages_String[] = {"doneCal", "openingCal", "closingCal"};
calStages deviceCalStage = doneCal;
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state
float currentSpeed = stepper1.speed();
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

//starting positions (guesses) for the limit switches.  FULL_SWING should be set to be a little more than full travel.
int motorPosAtCCW = -FULL_SWING;
int motorPosAtCW = FULL_SWING;
bool motorPosAtCCWset = false;
bool motorPosAtCWset = false;
int motorPosAtFullCCW = -FULL_SWING; //used to store actual limit SW position for reference
int motorPosAtFullCW = FULL_SWING;   //used to store actual limit SW position for reference

//make a struct of type fishyDevice to hold data on devices on the net; and if this is the master node
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
	int motorPosAtFullCCW = -FULL_SWING;
	int motorPosAtFullCW = FULL_SWING;
	bool openIsCCW = true;
	bool isMaster;
	bool dead = true;
	String group = "";
	String note = "";
	String swVer = "";
	String devType = "";
} fishyDevice;

//188 byte struct for storing personailty data in EEPROM
//remember a byte is needed for the string terminations
#define EEPROMsz 188
struct EEPROMdata
{
	char initstr[13] = "";
	char namestr[41] = "";
	bool master = 0;
	char typestr[21] = "";
	char groupstr[41] = "";
	char note[56] = "";
	char openIsCCW[4] = "";
	char swVer[11] = "";
} EEPROMdata;

fishyDevice deviceArray[MAX_DEVICE];
IPAddress masterIP = {0, 0, 0, 0};

// ----------------------------------------------------------------------------------------
// Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable
// ----------------------------------------------------------------------------------------
fauxmoESP fauxmo;

// -----------------------------------------------------------------------------
// WiFi
// -----------------------------------------------------------------------------
void WiFiSetup()
{
	WiFiManager WiFiManager;

	//reset saved settings (for testing)--------------------------------------------
	//WiFiManager.resetSettings();
	//------------------------------------------------------------------------------

	//if SSID and Password haven't been saved from before this opens an AP from the device
	//allowing you to connect to the device from a phone/computer by joining its "network"
	//from your wifi list - the name of the network will be the device name.
	//After first configuration it will auto connect unless things fail and it needs to be reset
	WiFiManager.autoConnect(EEPROMdata.namestr);

	if (DEBUG_MESSAGES)
	{
		Serial.println();
	}

	// Connected!
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[WiFi] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
	}

	Udp.begin(UDP_LOCAL_PORT); //start listening on UDP port for node-node traffic
	UDPbroadcast();			   //get a poll going

	String hostName;
	if (EEPROMdata.master)
	{
		hostName = "fishyDIY";
	}
	else
	{
		hostName = "fishyDIYNode" + String(WiFi.localIP()[3]);
	}
	MDNS.begin(hostName.c_str());					  //start mDNS to fishyDIYmaster.local
	httpServer.on("/genericArgs", handleGenericArgs); //Associate the handler function to the path
	httpServer.on("/", handleRoot);
	//httpServer.on("/controlPanel", handleControlPanel);
	httpServer.on("/dataDump", dataDump);
	httpServer.on("/device.JSON", handleJSON);
	httpServer.onNotFound(handleNotFound);
	httpUpdater.setup(&httpServer);
	httpServer.begin();

	MDNS.addService("http", "tcp", 80);
	fastBlinks(5);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("%s is now ready! Open http://%s.local/update in your browser\n", EEPROMdata.namestr, hostName.c_str());
	}
}

//this is the base setup routine called first
void setup()
{
	// Initialize serial port and clean garbage
	if (DEBUG_MESSAGES)
	{
		Serial.begin(SERIAL_BAUDRATE);
		Serial.println();
		Serial.println();
	}

	//Setup Device Personaility and update EEPROM data as needed

	uint addr = 0;

	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] The personality data needs " + String(sizeof(EEPROMdata)) + " Bytes in EEPROM.");
	}

	EEPROM.begin(EEPROMsz);

	// load EEPROM data into RAM, see it
	EEPROM.get(addr, EEPROMdata);
	

	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Found: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW)+", SW Version string: "+String(EEPROMdata.swVer));

		Serial.println("[SETUP] New init string: " + String(INITIALIZE) + ". Stored init string: " + String(EEPROMdata.initstr));
	}
	//always show the latest SW_VER
	strncpy(EEPROMdata.swVer, SW_VER, 11);
	if (DEBUG_MESSAGES)
	{		Serial.println("[SETUP] Actual swVER: " + String(EEPROMdata.swVer));}

	// change EEPROMdata in RAM
	if (String(INITIALIZE) != String(EEPROMdata.initstr))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Updating.");
		}
		strncpy(EEPROMdata.initstr, INITIALIZE, 13);
		strncpy(EEPROMdata.namestr, CUSTOM_DEVICE_NAME, 41);
		strncpy(EEPROMdata.groupstr, CUSTOM_GROUP_NAME, 41);
		strncpy(EEPROMdata.typestr, CUSTOM_DEVICE_TYPE, 21);
		strncpy(EEPROMdata.note, CUSTOM_NOTE, 56);
		strncpy(EEPROMdata.openIsCCW, OPEN_IS_CCW_OR_CW, 4);
		
		if (MASTER_NODE)
		{
			if (DEBUG_MESSAGES)
			{
				Serial.println("[SETUP] Setting as MASTER.");
			}
			EEPROMdata.master = true;
		}
		else
		{
			EEPROMdata.master = false;
		}
		// replace values in EEPROM
		EEPROM.put(addr, EEPROMdata);
		EEPROM.commit();
		// reload EEPROMdata for EEPROM, see the change
		EEPROM.get(addr, EEPROMdata);
	}
	else
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Nothing to update.");
		}
	}

	if (DEBUG_MESSAGES)
	{Serial.println("Personality values are: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW)+", SW Version string: "+String(EEPROMdata.swVer));
	}

	//stepper motor setup
	stepper1.setMaxSpeed(MAX_SPEED);
	stepper1.setAcceleration(ACCELERATION);
	stepper1.setSpeed(0);

	// You can enable or disable the library at any moment
	// Disabling it will prevent the devices from being discovered and switched
	fauxmo.enable(false);

	if (!DEBUG_WiFiOFF)
	{
		// WiFi
		WiFiSetup();
		fauxmo.enable(true);
		// Add virtual device
		fauxmo.addDevice(EEPROMdata.namestr);
	}

	// Device and pin setup
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[SETUP] Device: %s; deviceState: %s.\n", EEPROMdata.namestr, deviceState ? "ON" : "OFF");
	}

	// Set set of device to off
	deviceState = LOW;

	//set switch pins to use internal pull_up resistor
	pinMode(SWpinLimitCW, INPUT_PULLUP);
	pinMode(SWpinLimitCCW, INPUT_PULLUP);
	pinMode(SWpinManCW, INPUT_PULLUP);
	pinMode(SWpinManCCW, INPUT_PULLUP);
	pinMode(SWpinManSel, INPUT_PULLUP);

	// fauxmoESP 2.0.0 has changed the callback signature to add the device_id,
	// this way it's easier to match devices to action without having to compare strings.
	fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state) {
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[SETUP] Device #%d (%s) state was set: %s\n", device_id, device_name, state ? "ON" : "OFF");
		}
		executeState(state);

	});

	// Callback to retrieve current state (for GetBinaryState queries)
	fauxmo.onGetState([](unsigned char device_id, const char *device_name) {
		return deviceState;
	});

	//announce master if this is the mastr node
	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	}
}

//process UDP packets
void UDPprocessPacket()
{
	//USED FOR UDP COMMS
	char packetBuffer[MAXCMDSZ]; //buffer to hold incoming packet

	// if there's data available, read a packet
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		IPAddress remoteIp = Udp.remoteIP();
		if (DEBUG_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Received packet of size ");
			Serial.println(packetSize);
			Serial.print("[UDPprocessPacket] From ");
			Serial.print(remoteIp);
			Serial.print(", port ");
			Serial.println(Udp.remotePort());
		}
		// read the packet into packetBufffer
		int len = Udp.read(packetBuffer, MAXCMDSZ);
		if (len > 0)
		{
			packetBuffer[len] = 0;
		}
		if (DEBUG_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Executing:");
			Serial.println(packetBuffer);
		}
		executeCommands(packetBuffer, remoteIp);
	}
}

//take an address for a node and figure out what to do with it
//return the index on success or -1 on fail
int dealWithThisNode(fishyDevice netDevice)
{
	if (EEPROMdata.master)
	{
		int index = findNode(netDevice.ip);
		if (index > -1)
		{
			updateNode(index, netDevice);
			if (DEBUG_MESSAGES)
			{
				Serial.print("[dealWithThisNode] updated ");
				Serial.print(netDevice.isMaster ? "[MASTER] " : "");
				Serial.println(netDevice.name + " @ " + netDevice.ip.toString());
			}
			return index;
		}
		else
		{
			if (DEBUG_MESSAGES)
			{
				Serial.print("[dealWithThisNode] added.. ");
				Serial.print(netDevice.isMaster ? "[MASTER] " : "");
				Serial.println(netDevice.name + " @ " + netDevice.ip.toString());
			}
			return storeNewNode(netDevice);
		}
	}
}

//lookup the index for an IPAddress in the deviceArray---------------------------
//returns -1 if not found. Otherwise returns the index.
//first verifies this is the EEPROMdata.master since other nodes have a zero size array
int findNode(IPAddress lookupIP)
{
	if (EEPROMdata.master)
	{
		for (int i = 0; i < MAX_DEVICE; i++)
		{
			if (!deviceArray[i].dead)
			{
				if (deviceArray[i].ip == lookupIP)
				{
					return i;
				}
			}
		}
		return -1;
	}
}

//lookup the index for the first dead (empty) device ---------------------------
//returns -1 if not found. Otherwise returns the index.
//first verifies this is the EEPROMdata.master since other nodes have a zero size array
int findDeadNode()
{
	if (EEPROMdata.master)
	{
		for (int i = 0; i < MAX_DEVICE; i++)
		{
			if (deviceArray[i].dead)
			{
				return i;
			}
		}
		return -1; //no more room - oh well
	}
}

//store an updated device in the deviceArray at index
void updateNode(int index, fishyDevice updatedDevice)
{
	if (EEPROMdata.master)
	{
		deviceArray[index] = updatedDevice;
	}
}

//store a new device's data in the deviceArray
int storeNewNode(fishyDevice newDevice)
{
	if (EEPROMdata.master)
	{
		int index = findDeadNode();
		if (index > -1)
		{
			deviceArray[index] = newDevice;
			return index;
		}
		else
			return -1;
	}
}

//return a fishyDevice with this devices status in it
fishyDevice makeMyFishyDevice()
{
	fishyDevice holder;
	//fill with current data
	holder.dead = false;
	holder.ip = WiFi.localIP();
	if (motorPosAtCCWset && motorPosAtCWset)
	{
		holder.isCalibrated = true;
	}
	else
	{
		holder.isCalibrated = false;
	}
	holder.isMaster = EEPROMdata.master;
	holder.motorPos = stepper1.currentPosition();
	holder.motorPosAtCCW = motorPosAtCCW;
	holder.motorPosAtCW = motorPosAtCW;
	holder.motorPosAtFullCCW = motorPosAtFullCCW;
	holder.motorPosAtFullCW = motorPosAtFullCW;
	holder.name = String(EEPROMdata.namestr);
	holder.port = UDP_LOCAL_PORT;
	holder.isMaster = EEPROMdata.master;
	holder.devType = String(EEPROMdata.typestr);
	holder.group = String(EEPROMdata.groupstr);
	holder.note = String(EEPROMdata.note);
	holder.swVer = String(EEPROMdata.swVer);
	if (OPEN_IS_CCW_OR_CW == "CCW")
	{
		holder.openIsCCW = true;
	}
	else
	{
		holder.openIsCCW = false;
	}

	return holder;
}
//broadcast on subnet to see who will respond
void UDPbroadcast()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;

	//process this devices data first, storing it in the deviceArray, then poll the rest of the network
	dealWithThisNode(makeMyFishyDevice());

	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("ANYFISHYDEV_THERE");
	//UDPdataDump();
	Udp.endPacket();
}

void UDPannounceMaster()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;
	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("FISHYDIYMASTER");
	Udp.endPacket();
}

//put out this devices data on the net
void UDPpollReply(IPAddress remote)
{

	fishyDevice holder; //temp
	holder = makeMyFishyDevice();

	String response = "POLL_RESPONSE ";

	Udp.beginPacket(remote, UDP_LOCAL_PORT);

/* 
send fishyDevice data.
Note - this is parsed by UDPparsePollResponse and paralleled by getJSON; 
if adding elements all three need updating.
{ip,isCalibrated,isMaster,motorPos,motorPosAtCCW,motorPosAtCW,motorPosAtFullCCW,motorPosAtFullCW,name,openIsCCW,port,group,note,swVer,devType}
*/
	response += "{" + holder.ip.toString() + "," + 
			String(holder.isCalibrated ? "true" : "false") + "," +
			String(holder.isMaster ? "true" : "false") + "," + 
			String(holder.motorPos) + "," +
			String(holder.motorPosAtCCW) + "," + 
			String(holder.motorPosAtCW) + "," + 
			String(holder.motorPosAtFullCCW) + "," +
			String(holder.motorPosAtFullCW) + "," + 
			String(holder.name) + ","  +  
			String(holder.openIsCCW ? "true" : "false") + "," + 
			String(holder.port) + "," + 
			String(holder.group) + "," +
			String(holder.note) + "," + 
			String(holder.swVer) + "," + 
			String(holder.devType) + "}";

	Udp.write(response.c_str());
	Udp.endPacket();

	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	} //make sure they know who is in charge
}

//parses a UDP poll reply and takes action
//TODO - parse the response further and add new nodes to the list and drop off old nodes
void UDPparsePollResponse(String responseIn, IPAddress remote)
{
	if (EEPROMdata.master)
	{
/* 
parse fishyDevice data.
Note - this data set is sent by UDPparsePollResponse and getJSON; 
it is also parsed by scripts in webresources.h if adding elements all three need updating:
{ip,isCalibrated,isMaster,motorPos,motorPosAtCCW,motorPosAtCW,motorPosAtFullCCW,motorPosAtFullCW,name,openIsCCW,port,group,note,swVer,devType}
*/
		String response = responseIn.substring(14); //strip off "POLL RESPONSE"
		fishyDevice holder;
		holder.dead = false;

		//IP
		int strStrt = 1;
		int strStop = response.indexOf(",", strStrt);
		String strIP = response.substring(strStrt, strStop);
		holder.ip[3] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[2] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[1] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[0] = strIP.toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strIP: " + holder.ip.toString());
		}

		//isCalibrated
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.isCalibrated = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] isCalibrated: ");
			Serial.println(holder.isCalibrated ? "true" : "false");
		}

		//isMaster
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.isMaster = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] isMaster: ");
			Serial.println(holder.isMaster ? "true" : "false");
		}

		//motorPos
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPos = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] motorPos: " + String(holder.motorPos));
		}

		//motorPosAtCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtCCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] motorPosAtCCW: " + String(holder.motorPosAtCCW));
		}

		//motorPosAtCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtCW: " + String(holder.motorPosAtCW));
		}

		//motorPosAtFullCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtFullCCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtFullCCW: " + String(holder.motorPosAtFullCCW));
		}

		//motorPosAtFullCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtFullCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtFullCW: " + String(holder.motorPosAtFullCW));
		}

		//name
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.name = response.substring(strStrt, strStop);
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strName: " + holder.name);
		}

		//openIsCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.openIsCCW = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] openIsCCW: ");
			Serial.println(holder.openIsCCW ? "true" : "false");
		}

		//port
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.port = response.substring(strStrt, strStop - 1).toInt(); //drop "}"
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strPort: " + String(holder.port));
		}

		//group
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.group = response.substring(strStrt, strStop - 1).toInt(); //drop "}"
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strGroup: " + String(holder.group));
		}

		//note
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.note = response.substring(strStrt, strStop - 1).toInt(); //drop "}"
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strnote: " + String(holder.note));
		}

		//swVer
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.swVer = response.substring(strStrt, strStop - 1).toInt(); //drop "}"
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strswVer: " + String(holder.swVer));
		}

		//devType
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.devType = response.substring(strStrt, strStop - 1).toInt(); //drop "}"
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strdevType: " + String(holder.devType));
		}

		dealWithThisNode(holder);
	}
}

void UDPparseConfigResponse(String responseIn, IPAddress remote){
	String response = responseIn.substring(7); //strip off "CONFIG"
	int strStrt, strStop;

/*
		parseString in this order: {ccwLim, cwLim, openIsCCW, isMaster, devName, groupName, devType, note}
		strncpy(EEPROMdata.initstr, INITIALIZE, 13);
		strncpy(EEPROMdata.namestr, CUSTOM_DEVICE_NAME, 41);
		strncpy(EEPROMdata.groupstr, CUSTOM_GROUP_NAME, 41);
		strncpy(EEPROMdata.typestr, CUSTOM_DEVICE_TYPE, 21);
		strncpy(EEPROMdata.note, CUSTOM_NOTE, 56);
		strncpy(EEPROMdata.openIsCCW, OPEN_IS_CCW_OR_CW, 4);
*/

	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] Got this: " + responseIn);
	}	
	//ccwLim
	strStrt = response.indexOf("=", 1)+1;
	strStop = response.indexOf(";", strStrt);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] ccwLim: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	//cwLim
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] cwLim: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	//openIsCCW
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] openIsCCW: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	strncpy(EEPROMdata.openIsCCW, (response.substring(strStrt, strStop) == "false") ? "CW" : "CCW", 4);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] openIsCCW: ");
		Serial.println(EEPROMdata.openIsCCW ? "true" : "false");
	}
	//isMaster
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] isMaster: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	EEPROMdata.master = (response.substring(strStrt, strStop) == "false") ? false : true;
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] isMaster: ");
		Serial.println(EEPROMdata.master ? "true" : "false");
	}
	//devName
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.namestr, response.substring(strStrt, strStop).c_str(), 41);
	//EEPROMdata.namestr = response.substring(strStrt, strStop);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] strName: " + String(EEPROMdata.namestr));
	}
	//groupName
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.groupstr, response.substring(strStrt, strStop).c_str(), 41);
	//EEPROMdata.groupstr = response.substring(strStrt, strStop);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] strName: " + String(EEPROMdata.groupstr));
	}
	//devType
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.typestr, response.substring(strStrt, strStop).c_str(), 21);
	//EEPROMdata.typestr = response.substring(strStrt, strStop);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] strName: " + String(EEPROMdata.typestr));
	}
	//note
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.note, response.substring(strStrt, strStop).c_str(), 56);
	//EEPROMdata.note = response.substring(strStrt, strStop);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] strName: " + String(EEPROMdata.note));
	}

	//TODO - I'm HERE - need to test the EEPROM storage
	// - need to test masterStr == 0.0.0.0 allowing serving

	uint addr = 0;
	EEPROM.begin(EEPROMsz);
	// replace values in EEPROM
	EEPROM.put(addr, EEPROMdata);
	EEPROM.commit();
}

//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
	int newPercentOpen = whichWayGoto(cmd.substring(4).toInt()); //STRIP OFF GOTO
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeGoto] Going to percent open: %d\n", newPercentOpen);
	}
	deviceTrueState = openPercentActuator(newPercentOpen);
}
//execute a WiFi received state change
void executeState(bool state)
{
	bool correctedState =whichWay(state);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (correctedState)
	{
		deviceTrueState = openActuator();
	}
	else
	{
		deviceTrueState = closeActuator();
	}
}

//do a manual move in the CW direction
void manCW()
{
	int CWlimSensorVal = digitalRead(SWpinLimitCW);
	int curPos = stepper1.currentPosition();
	manSelCnt = 0;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[manCW] CWlim: %d; motorPos: %d; lastCommand: %s\n", CWlimSensorVal, curPos, manCommandState_String[lastManCommand]);
	}

	lastManCommand = CW;

	if (CWlimSensorVal == 1)
	{ //not at CW limit
		deviceTrueState = man_CW;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[manCW] ..moving CW..\n");
		}

		if (!stepper1.isRunning())
		{
			stepper1.enableOutputs();
		}

		stepper1.move(MANSTEP);
		stepper1.setSpeed(MANSPEED);
		stepper1.runSpeedToPosition();
	}
	else
	{
		//at actual SW limit update the limit SW position
		motorPosAtFullCW = curPos;
		deviceTrueState = man_idle;
		idleActuator(deviceTrueState);
	}
}

//do a manual move in the CCW direction

void manCCW()
{
	int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
	int curPos = stepper1.currentPosition();
	manSelCnt = 0;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[manCCW] CCWlim: %d; motorPos: %d; lastCommand: %s\n", CCWlimSensorVal, curPos, manCommandState_String[lastManCommand]);
	}

	lastManCommand = CCW;

	if (CCWlimSensorVal == 1)
	{ //not at CCW limit
		deviceTrueState = man_CCW;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[manCCW] ..moving CCW..\n");
		}

		if (!stepper1.isRunning())
		{
			stepper1.enableOutputs();
		}

		stepper1.move(-MANSTEP);
		stepper1.setSpeed(-MANSPEED);
		stepper1.runSpeedToPosition();
	}
	else
	{
		//at actual SW limit update the limit SW position
		motorPosAtFullCCW = curPos;
		deviceTrueState = man_idle;
		idleActuator(deviceTrueState);
	}
}

//set new limit for stopping motor based on the last commanded manual position
void manSel()
{
	int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
	int CWlimSensorVal = digitalRead(SWpinLimitCW);
	int curPos = stepper1.currentPosition();

	//if button pressed for more than 1 second increment the count
	static unsigned long last = millis();
	if (millis() - last > 1000)
	{
		last = millis();
		manSelCnt++;
	}

	if (DEBUG_MESSAGES)
	{
		Serial.printf("[manSel] CCWlim: %d; CWlim: %d; motorPos: %d; lastCommand: %s; manSelCnt: %d;\n", CCWlimSensorVal, CWlimSensorVal, curPos, manCommandState_String[lastManCommand], manSelCnt);
	}
	switch (lastManCommand)
	{
	case CW: //set new CW limit position
		if (!CWlimSensorVal)
		{
			setCWLimit(curPos - 3); //back off of limit switch a bit
		}
		else
		{
			setCWLimit(curPos);
		}
		break;
	case CCW: //set new CCW limit position
		if (!CCWlimSensorVal)
		{
			setCCWLimit(curPos + 3); //back off of limit switch a bit
		}
		else
		{
			setCCWLimit(curPos);
		}
		break;
	case none:
	case SEL:
		if (manSelCnt > 5)
		{ //select button held long time - reset the limits and put the deviceState -> unknown
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[manSEL] Long Press detected - reseting the box\n");
			}
			resetController();
		}
		break;
	}
	lastManCommand = SEL;
}

void setCWLimit(long newLimit)
{
	motorPosAtCW = newLimit;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[setCWLimit] New Limit: %d \n", newLimit);
	}
	motorPosAtCWset = true;
	if (motorPosAtCCW >= motorPosAtCW)
	{ //if limits are reversed, update the CCW limit
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[setCWLimit] LIMITS REVERSED, updating CCW Limit: %d ", newLimit - 1);
		}
		motorPosAtCCW = motorPosAtCW - 1;
		motorPosAtCCWset = true;
	}
}

void setCCWLimit(long newLimit)
{
	motorPosAtCCW = newLimit;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[setCCWLimit] New Limit: %d \n", newLimit);
	}
	motorPosAtCCWset = true;
	if (motorPosAtCW <= motorPosAtCCW)
	{ //if limits are reversed, update the CW limit
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[setCCWLimit] LIMITS REVERSED, updating CW Limit: %d \n", newLimit + 1);
		}
		motorPosAtCW = motorPosAtCCW + 1;
		motorPosAtCCWset = true;
	}
}

//function used to do a normal WiFi or calibration opening of the actuator
void normalOpening()
{
	if (!digitalRead(SWpinLimitCCW))
	{
		//reached limit - disable everything and reset motor to idle
		motorPosAtFullCCW = stepper1.currentPosition();
		motorPosAtCCW = motorPosAtFullCCW + 3; //back off of limit switch a bit
		motorPosAtCCWset = true;

		if (!motorPosAtCWset)
		{ //if opposite position not set, set guess at full swing
			motorPosAtCW = motorPosAtCCW + FULL_SWING;
		}
		idleActuator(opened); //make actuator idle
		if (DEBUG_MESSAGES)
		{
			int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
			int CWlimSensorVal = digitalRead(SWpinLimitCW);
			Serial.printf("[normalOpening] Reached CCW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}
	else
	{ //keep going if still have distance to travel
		if (stepper1.distanceToGo() != 0)
		{
			stepper1.run();
		}
		else
		{
			idleActuator(opened);
		}
	}
}

//function used to do a normal WiFi or calibration closing of the actuator
void normalClosing()
{
	if (!digitalRead(SWpinLimitCW))
	{
		//reached limit - disable everything and reset motor to idle
		motorPosAtFullCW = stepper1.currentPosition();
		motorPosAtCW = motorPosAtFullCW - 3; //back off of limit switch a bit
		motorPosAtCWset = true;

		if (!motorPosAtCCWset)
		{ //if opposite position not set, set guess at full swing
			motorPosAtCCW = motorPosAtCW - FULL_SWING;
		}
		idleActuator(closed); //make actuator idle
		if (DEBUG_MESSAGES)
		{
			int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
			int CWlimSensorVal = digitalRead(SWpinLimitCW);
			Serial.printf("[normalClosing] Reached CW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}
	else
	{ //keep going if still have distance to travel
		if (stepper1.distanceToGo() != 0)
		{
			stepper1.run();
		}
		else
		{
			idleActuator(closed);
		}
	}
}

//put the controller in a unknown state ready for calibration
void resetController()
{
	ESP.restart();
	//ESP.reset();
}

//return data out for troubleshooting
void dataDump()
{
	char temp[500];
	int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
	int CWlimSensorVal = digitalRead(SWpinLimitCW);
	int CWmanSensorVal = digitalRead(SWpinManCW);
	int CCWmanSensorVal = digitalRead(SWpinManCCW);
	int SELmanSensorVal = digitalRead(SWpinManSel);
	long curPos = stepper1.currentPosition();
	long distToGo = stepper1.distanceToGo();
	float curSpeed = stepper1.speed();

	sprintf(temp, "[DATA DUMP]\nSwitches -     LimCCW=%d LimCW=%d; ManCCW=%d ManCW=%d ManSEL=%d\nMotor -        "
				  "Pos=%d Runn'g?=%s Speed=%d toGo=%d \nLimits Set -   CCW=%s (%d) CW=%s (%d)\nDeviceStates - WiFi=%s   True=%s   Cal=%s\n",
			CCWlimSensorVal, CWlimSensorVal, CCWmanSensorVal, CWmanSensorVal, SELmanSensorVal, curPos, stepper1.isRunning() ? "Y" : "N", curSpeed, distToGo, motorPosAtCCWset ? "Y" : "N", motorPosAtCCW, motorPosAtCWset ? "Y" : "N", motorPosAtCW, deviceState ? "ON" : "OFF", trueState_String[deviceTrueState], calStages_String[deviceCalStage]);

	httpServer.send(200, "text/html", temp);
	//if(DEBUG_MESSAGES){Serial.println(temp);}
	/*
	String str = "";
	Dir dir = SPIFFS.openDir("/");
	Serial.println("dir" + dir.fileName());
	while (dir.next()) {
		str = "<br>";
		str += dir.fileName();
		str += " / ";
		str += dir.fileSize();
		str += "\r\n";
		//httpServer.send(200, "text/html", str);
		Serial.println("dir" + dir.fileName());
	}
	*/
}

//TEST - REMOVE ------------------------------------
//print data out for troubleshooting
void UDPdataDump()
{
	int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
	int CWlimSensorVal = digitalRead(SWpinLimitCW);
	int CWmanSensorVal = digitalRead(SWpinManCW);
	int CCWmanSensorVal = digitalRead(SWpinManCCW);
	int SELmanSensorVal = digitalRead(SWpinManSel);
	long curPos = stepper1.currentPosition();
	long distToGo = stepper1.distanceToGo();
	float curSpeed = stepper1.speed();
	char reply[255];

	sprintf(reply, "[UDP DATA DUMP]\nSwitches -     LimCCW=%d LimCW=%d; ManCCW=%d ManCW=%d ManSEL=%d", CCWlimSensorVal, CWlimSensorVal, CCWmanSensorVal, CWmanSensorVal, SELmanSensorVal);
	Udp.write(reply);
}
//--------------------------------------------------

void loop()
{

	//handle webrequests
	httpServer.handleClient();

	//process any UDP traffic
	UDPprocessPacket();

	//announce the master every 60 seconds
	if (EEPROMdata.master)
	{
		static unsigned long lastMstr = millis();
		if (millis() - lastMstr > 60000)
		{
			lastMstr = millis();
			Serial.printf("[MAIN] Announcing master");
			UDPannounceMaster();
		}
	}

	// Since fauxmoESP 2.0 the library uses the "compatibility" mode by
	// default, this means that it uses WiFiUdp class instead of AsyncUDP.
	// The later requires the Arduino Core for ESP8266 staging version
	// whilst the former works fine with current stable 2.3.0 version.
	// But, since it's not "async" anymore we have to manually poll for UDP
	// packets
	fauxmo.handle();

	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal = digitalRead(SWpinLimitCCW);
	int CWlimSensorVal = digitalRead(SWpinLimitCW);
	int CWmanSensorVal = digitalRead(SWpinManCW);
	int CCWmanSensorVal = digitalRead(SWpinManCCW);
	int SELmanSensorVal = digitalRead(SWpinManSel);

	//STATE MACHINE:
	//First - if manual control commanded - jump to it and execute. Abort Calibration or WiFi state change if in progress.
	//Second - check for a calibration in progress; if so - finish calibration.
	//Last - go through all the other actions based on deviceTrueState.
	if ((CWmanSensorVal + CCWmanSensorVal + SELmanSensorVal) == 3)
	{ //New MANUAL CONTROL not ordered - all three are HIGH (switches open)
		switch (deviceCalStage)
		{
		case doneCal:
			//No new manual ctrl ordered or calibration - use deviceTrueState to determine actions
			//advance the stepper if opening or closing and not at a limit (or at the correct stopping position)
			switch (deviceTrueState)
			{
			case man_idle: //idle - do nothing
			case opened:   //idle - do nothing
			case closed:   //idle - do nothing
				break;
			case man_CCW: //opening manually
				if (CCWmanSensorVal == 1)
				{ //was manually opening, now done
					idleActuator(man_idle);
				}
				else
				{ //run
					stepper1.runSpeedToPosition();
				}
				break;
			case man_CW: //closing manually
				if (CWmanSensorVal == 1)
				{ //was manually closing, now done
					idleActuator(man_idle);
				}
				else
				{ //run
					stepper1.runSpeedToPosition();
				}
				break;
			case opening: //opening not at limit
				normalOpening();
				break;
			case closing: //closing not at limit
				normalClosing();
				break;
			case unknown: //unknown state (bootup)
				if (!CWlimSensorVal)
				{ //at CW limit
					motorPosAtFullCW = stepper1.currentPosition();
					motorPosAtCW = motorPosAtFullCW - 3; //back off of limit switch a bit
					motorPosAtCWset = true;
					motorPosAtCCW = motorPosAtCW - FULL_SWING;
					motorPosAtCCWset = false;
					idleActuator(closed); //make actuator idle
				}
				else if (!CCWlimSensorVal)
				{ //at CCW limit
					motorPosAtFullCCW = stepper1.currentPosition();
					motorPosAtCCW = motorPosAtFullCCW + 3; //back off of limit switch a bit
					motorPosAtCCWset = true;
					motorPosAtCW = motorPosAtCCW + FULL_SWING;
					motorPosAtCWset = false;
					idleActuator(opened); //make actuator idle
				}
				else
				{
					motorPosAtCWset = false;
					motorPosAtCCWset = false;
				}
				break;
			}
			break;
		case openingCal: //calibration in first stage
			normalOpening();
			if (deviceTrueState == opened)
			{ //Done with stage 1 go onto closing stage
				deviceCalStage = closingCal;
				if (DEBUG_MESSAGES)
				{
					Serial.printf("[MAIN loop cal] Found open (CCW) limit (%d). Going to calibration closing stage.\n", motorPosAtCCW);
				}
				executeState(false);
			}
			break;
		case closingCal: //calibration in second stage
			normalClosing();
			if (deviceTrueState == closed)
			{ //Done with stage 2 - done!
				deviceCalStage = doneCal;
				if (DEBUG_MESSAGES)
				{
					Serial.printf("[MAIN loop cal] Found close (CW) limit (%d). Calibration complete.\n", motorPosAtCW);
				}
			}
			break;
		}
	}
	else
	{ //manual controls in use
		if (deviceCalStage != doneCal)
		{
			deviceCalStage = doneCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN man ctrl] CANCELLED CALIBRATION.\n");
			}
		}
		if (CWlimSensorVal && (CWmanSensorVal == 0))
		{ //go CW since not at limit
			manCW();
		}
		else if (CCWlimSensorVal && (CCWmanSensorVal == 0))
		{ //go CCW since not at limit
			manCCW();
		}
		else if (SELmanSensorVal == 0)
		{
			manSel();
		}
	}

	if (DEBUG_MESSAGES)
	{
		//take serial commands in
		if (Serial.available() > 0)
		{
			char inputMsg[MAXCMDSZ];
			int sizeRead;
			sizeRead = Serial.readBytesUntil('\r', inputMsg, sizeof(inputMsg));
			if (sizeRead)
			{
				executeCommands(inputMsg, IPAddress(0, 0, 0, 0));
			}
			Serial.flush();
		}

		if (DEBUG_HEAP_MESSAGE)
		{
			static unsigned long last = millis();
			if (millis() - last > 1000)
			{
				last = millis();
				Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
				//print out the value of the limit switches
				Serial.printf("[MAIN] TrueState: %d, POS: %d, CW_LIM_SET? %s,CCW_LIM_SET? %s\n", deviceTrueState, stepper1.currentPosition(), motorPosAtCCWset ? "Y" : "N", motorPosAtCWset ? "Y" : "N");
				Serial.printf("[MAIN] Manual Switch Positions CCWman: %d; CWman: %d; SELman: %d\n", CCWmanSensorVal, CWmanSensorVal, SELmanSensorVal);
			}
		}
	}
}
//this function replaces open with close if 
//CW is defined as open by openIsCCW
bool whichWay(String in){
	bool ret = (in=="open")?true:false;
	if(EEPROM.openIsCCW=="CCW"){
		ret = !ret;
	}
	return ret;
}
//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as 
//open by openIsCCW
int whichWayGoto(int in){
	String ret=in;
	if(EEPROM.openIsCCW=="CCW"){
		ret = 100-ret;
	}
	return ret;
}
void executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
	String cmd = String(inputMsg);
	cmd.toLowerCase();
	//Serial.println("[executeCommands]" + cmd);
	if (cmd.startsWith("open"))
	{ //TODO - make open determine CCW or CW
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded OPEN");
		}
		executeState(true); //WiFi "true" is open or going CCW to stop
	}
	else if (cmd.startsWith("close"))
	{ //TODO - make open determine CCW or CW
		Serial.println("[executeCommands] Commanded CLOSE");
		executeState(false); //WiFi "false" is close or going CW to stop
	}
	else if (cmd.startsWith("goto"))
	{ //TODO - make open determine CCW or CW
		Serial.println("[executeCommands] Commanded GOTO (" + cmd + ")");
		executeGoto(cmd); //WiFi "false" is close or going CW to stop
	}
	else if (cmd.startsWith("data"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded DATA");
		}
		//TODO - make it work for serial also
		//dataDump();
	}
	else if (cmd.startsWith("reset"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET");
		}
		resetController();
	}
	else if (cmd.startsWith("calibrate"))
	{ //TODO - rename this "test full range"
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CALIBRATE");
		}
		deviceCalStage = openingCal;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[executeCommands] Going to calibration opening stage.\n");
		}
		executeState(true);
	}
	else if (cmd.startsWith("anyfishydev_there"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded ANYFISHYDEV_THERE");
		}
		UDPpollReply(remote);
	}
	else if (cmd.startsWith("poll_net"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_NET");
		}
		UDPbroadcast();
	}
	else if (cmd.startsWith("poll_response"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_RESPONSE");
		}
		UDPparsePollResponse(String(inputMsg), remote); //want the original case for this
	}
	else if (cmd.startsWith("fishydiymaster"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded FISHYDIYMASTER");
		}
		masterIP = remote; //update the master IP
	}
	else if (cmd.startsWith("config"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CONFIG");
		}
		UDPparseConfigResponse(String(inputMsg), remote); //want the original case for this
	}
	else
	{
		Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);
	}
}

trueState idleActuator(trueState idleState)
{
	stepper1.stop();
	stepper1.setSpeed(0);
	stepper1.disableOutputs();
	deviceTrueState = idleState;
	UDPpollReply(masterIP); //tell the Master Node the new info
	return idleState;
}

trueState openActuator()
{
	trueState newState;
	if (digitalRead(SWpinLimitCCW) == 1)
	{ //switch is open; not at CCW limit
		newState = opening;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[openActuator] OPENING -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), motorPosAtCCW);
		}
		stepper1.enableOutputs();
		stepper1.moveTo(motorPosAtCCW);
		stepper1.setSpeed(-START_SPEED);
	}
	else
	{
		motorPosAtFullCCW = stepper1.currentPosition();
		newState = idleActuator(opened);
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[openActuator] OPENING -> ALREADY OPENED. Pos: %d\n", stepper1.currentPosition());
		}
	}
	return newState;
}

//TODO - implement this for CCW is open, and make the opposite for CW is open
// explain things were designed from the perspective of CCW is open
//TODO - update trueState for the percent open state
//TODO - consider deleting the other open/closeActuator functions and just using 0 and 100%
//to do the same thing
trueState openPercentActuator(int percent)
{
	trueState newState;
	//TODO - if(CCWisOpen){ else flip the calc }
	int newMotorPos = (int)((motorPosAtCW - motorPosAtCCW) * percent / 100 + motorPosAtCCW);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[openPercentActuator] Going to %d percent open. New Pos: %d\n", percent, newMotorPos);
	}
	if (newMotorPos < stepper1.currentPosition())
	{ //NEED TO MOVE CCW
		if (digitalRead(SWpinLimitCCW) == 1)
		{ //switch is open; not at CCW limit
			newState = opening;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CCW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), motorPosAtCCW);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(-START_SPEED);
		}
		else
		{
			motorPosAtFullCCW = stepper1.currentPosition();
			newState = idleActuator(opened);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CCW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
	else
	{
		if (digitalRead(SWpinLimitCW) == 1)
		{ //switch is open; not at CW limit
			newState = closing;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), motorPosAtCW);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(START_SPEED);
		}
		else
		{
			motorPosAtFullCW = stepper1.currentPosition();
			newState = idleActuator(closed);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
}
trueState closeActuator()
{
	trueState newState;
	if (digitalRead(SWpinLimitCW) == 1)
	{ //switch is open; not at CW limit
		newState = closing;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[closeActuator] CLOSING -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), motorPosAtCW);
		}
		stepper1.enableOutputs();
		stepper1.moveTo(motorPosAtCW);
		stepper1.setSpeed(START_SPEED);
	}
	else
	{
		motorPosAtFullCW = stepper1.currentPosition();
		newState = idleActuator(closed);
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[closeActuator] CLOSING -> ALREADY CLOSED. Pos: %d\n", stepper1.currentPosition());
		}
	}
	return newState;
}

void fastBlinks(int numBlinks)
{

	pinMode(SWpinManSel, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(SWpinManSel, LOW);  // Turn the LED on by making the voltage LOW
		delay(100);						 // Wait for a second
		digitalWrite(SWpinManSel, HIGH); // Turn the LED off by making the voltage HIGH
		delay(200);						 // Wait for two seconds
	}
	pinMode(SWpinManSel, INPUT_PULLUP);
}

void slowBlinks(int numBlinks)
{

	pinMode(SWpinManSel, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(SWpinManSel, LOW);  // Turn the LED on by making the voltage LOW
		delay(1000);					 // Wait for a second
		digitalWrite(SWpinManSel, HIGH); // Turn the LED off by making the voltage HIGH
		delay(1000);					 // Wait for two seconds
	}
	pinMode(SWpinManSel, INPUT_PULLUP);
}

/*========================================
===========WEB SERVER FUNCTIONS===========
=========================================*/
//provide a JSON structure with all the deviceArray data
void handleJSON()
{
	if (EEPROMdata.master)
	{
		UDPbroadcast();
		//TODO - IF NEW JSON DEVICE NOT IN THE CONTROL THEN CTRL PANEL SHOULD BE TOLD TO REFRESH
		httpServer.send(200, "text/html", getJSON().c_str());
	}
	else
	{
		handleNotMaster();
	}
}

void handleNotMaster()
{
	String response = " <!DOCTYPE html> <html> <head> <title>fishDIY Device Network</title> <style> body {background-color: #edf3ff;font-family: \"Lucida Sans Unicode\", \"Lucida Grande\", sans-serif;}  .fishyHdr {align:center; border-radius:25px;font-size: 18px;     font-weight: bold;color: white;background: #3366cc; vertical-align: middle; } </style>  <body> <div class=\"fishyHdr\">Go to <a href=\"http://" + masterIP.toString() + "\"> Master (" + masterIP.toString() + ")</a></div> </body> </html> ";
	httpServer.send(200, "text/html", response.c_str());
}

String getJSON()
{
	String temp;
	temp = "{\"fishyDevices\":[";
	for (int i = 0; i < MAX_DEVICE; i++)
	{
		if (!deviceArray[i].dead)
		{
			if (i > 0)
			{
				temp += ",";
			}
/* 
put fishyDevice data in a string.
Note - this is parsed by scripts in webresources.h 
and paralleled by UDPpollReply; 
if adding elements all these need updating.
{ip,isCalibrated,isMaster,motorPos,motorPosAtCCW,motorPosAtCW,motorPosAtFullCCW,motorPosAtFullCW,name,openIsCCW,port,group,note,swVer,devType}
*/
			temp += "{\"deviceID\":\"" + String(i) + "\",\"IP\":\"" + deviceArray[i].ip.toString() + "\",\"dead\":\"" + String(deviceArray[i].dead) +
					"\",\"isCalibrated\":\"" + String(deviceArray[i].isCalibrated ? "true" : "false") +
					"\",\"isMaster\":\"" + String(deviceArray[i].isMaster ? "true" : "false") + "\",\"motorPos\":\"" + String(deviceArray[i].motorPos) +
					"\",\"motorPosAtCCW\":\"" + String(deviceArray[i].motorPosAtCCW) + "\",\"motorPosAtCW\":\"" + String(deviceArray[i].motorPosAtCW) +
					"\",\"motorPosAtFullCCW\":\"" + String(deviceArray[i].motorPosAtFullCCW) + "\",\"motorPosAtFullCW\":\"" + String(deviceArray[i].motorPosAtFullCW) +
					"\",\"deviceName\":\"" + String(deviceArray[i].name) + "\",\"openIsCCW\":\"" + String(deviceArray[i].openIsCCW ? "true" : "false") +
					"\",\"port\":\"" + String(deviceArray[i].port) + "\",\"group\":\"" + String(deviceArray[i].group) + "\",\"note\":\"" + String(deviceArray[i].note) + "\",\"swVer\":\"" + String(deviceArray[i].swVer) + "\",\"devType\":\"" + String(deviceArray[i].devType) + "\"}";
		}
	}
	temp += "]}";
	//if(DEBUG_MESSAGES){Serial.println("[getJSON] built: " + temp);}
	return temp;
}

void handleGenericArgs()
{ //Handler
	if (EEPROMdata.master)
	{
		IPAddress IPtoSend;
		String IPforCommand = "";
		char command[MAXCMDSZ] = "";
		String message = "Number of args received:";
		message += httpServer.args(); //Get number of parameters
		message += "\n";			  //Add a new line
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleGenericArgs] :" + message);
		}
		for (int i = 0; i < httpServer.args(); i++)
		{
			message += "Arg #" + String(i) + " -> "; //Include the current iteration value
			message += httpServer.argName(i) + ": "; //Get the name of the parameter
			message += httpServer.arg(i) + "\n";	 //Get the value of the parameter

			if (httpServer.argName(i) == "IPCommand")
			{
				String response = httpServer.arg(i);

				//IP
				int strStrt = 0;
				int strStop = response.indexOf(";", strStrt);
				String strIP = response.substring(strStrt, strStop);
				IPtoSend[3] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[2] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[1] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[0] = strIP.toInt();
				response = response.substring(strStop + 1);
				if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
				{
					Serial.println("[handleGenericArgs] strIP:response " + IPtoSend.toString() + ":" + response);
				}
				message += "[handleGenericArgs] strIP:response " + IPtoSend.toString() + ":" + response;
				
				response.toCharArray(command, MAXCMDSZ);
				if (IPtoSend == WiFi.localIP())
				{
					executeCommands(command, WiFi.localIP());
				}
				else
				{
					Udp.beginPacket(IPtoSend, UDP_LOCAL_PORT);
					Udp.write(response.c_str());
					Udp.endPacket();
				}
			}
		}
		httpServer.send(200, "text/plain", message); //Response to the HTTP request
	}
	else
	{
		handleNotMaster();
	}
}
void handleRoot()
{
	//only process the webrequest if you are the master or if there is no master
	if (EEPROMdata.master || masterIP.toString()=="0.0.0.0")
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleRoot] 1: ");
		}

		int szchnk = 2900;

		//LARGE STRINGS - Break response into parts
		httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
		httpServer.sendHeader("Pragma", "no-cache");
		httpServer.sendHeader("Expires", "-1");
		httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN); 
		httpServer.send(200, "text/html", "");

		//Send PART1:
		handleStrPartResp(WEBSTRPT1,szchnk);
		
		//Send JSON:
		handleStrPartResp(String("var fishyNetJSON ='" + getJSON() + "';"),szchnk);
		
		//Send PART2:
		handleStrPartResp(WEBSTRPT2,szchnk);

		//Send PART3:
		handleStrPartResp(WEBSTRPT3,szchnk);

		//Send PART4:
		handleStrPartResp(WEBSTRPT4,szchnk);
		
		httpServer.sendContent("");
		httpServer.client().stop();

		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleRoot]\n");
		}
	}
	else
	{
		handleNotMaster();
	}
}

//send a str that is part of a webresponse and break it into chunks of szchnk
void handleStrPartResp(String str,int szchnk){
	int strt = 0;
	int stp = 0;
	int len = str.length();
	if (DEBUG_MESSAGES)	{Serial.println("[handleRoot] PT1 str.len: " + String(len));}
	while (stp < len)
	{
		strt = stp;
		stp = strt + szchnk;
		if (stp > len)
		{
			stp = len;
		}
		httpServer.sendContent(str.substring(strt, stp).c_str());
		if (DEBUG_MESSAGES)
		{
			Serial.println(str.substring(strt, stp));
		}
	}
}

//web server response to other
void handleNotFound()
{
	handleNotMaster();
}

//makes the name string 255 in length and adds H3 tags (space after tags so ignored by browser)
String paddedH3Name(String name)
{
	String newName = "<H3>" + name + "</H3>";
	int growBy = 255 - name.length();
	for (int i = 0; i < growBy; i++)
	{
		newName += " ";
	}
	return newName;
}
//return motor position as a [%] of allowed range
String motPosStr(int intPadded, fishyDevice device)
{
	int relPos = device.motorPos - device.motorPosAtCCW;
	int range = device.motorPosAtCW - device.motorPosAtCCW;
	int answer;
	if (range > 0)
	{
		answer = (int)(100 * relPos / range);
		return paddedInt(intPadded, answer);
	}
	else
		return paddedInt(intPadded, 0);
}

String threeDigits(int i)
{
	String threeStr;
	if (i < 10)
	{
		threeStr = "00";
	}
	else if (i < 100)
	{
		threeStr = "0";
	}
	else
	{
		threeStr = "";
	}
	threeStr += String(i);
	return threeStr;
}

String paddedInt(int lengthInt, int val)
{
	String paddedStr = String(val);
	while (paddedStr.length() < lengthInt)
	{
		paddedStr += " ";
	}
	return paddedStr;
}

String paddedIntQuotes(int lengthInt, int val)
{
	String paddedStr = "'" + String(val) + "'";
	while (paddedStr.length() < lengthInt + 2)
	{
		paddedStr += " ";
	}
	return paddedStr;
}