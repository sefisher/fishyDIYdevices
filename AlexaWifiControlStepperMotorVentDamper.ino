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

Notes:
- Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable via updated Alexa devices
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
//#include "customHTTPUpdateServer.h"
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <fauxmoESP.h>
#include <AccelStepper.h>
#include <EEPROM.h>

#include "deviceDefinitions.h"
#include "motorDefinitions.h"
#include "globals.h"
#include "webresources.h"


//this is the base setup routine called first on power up or reboot
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
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] The personality data needs " + String(sizeof(EEPROMdata)) + " Bytes in EEPROM.");
		Serial.println("[SETUP] initstr " + String(sizeof(EEPROMdata.initstr)));
		Serial.println("[SETUP] namestr " + String(sizeof(EEPROMdata.namestr)));
		Serial.println("[SETUP] master " + String(sizeof(EEPROMdata.master)));
		Serial.println("[SETUP] typestr " + String(sizeof(EEPROMdata.typestr)));
		Serial.println("[SETUP] groupstr " + String(sizeof(EEPROMdata.groupstr)));
		Serial.println("[SETUP] note " + String(sizeof(EEPROMdata.note)));
		Serial.println("[SETUP] openIsCCW " + String(sizeof(EEPROMdata.openIsCCW)));
		Serial.println("[SETUP] swVer " + String(sizeof(EEPROMdata.swVer)));
		Serial.println("[SETUP] motorPosAtCCW " + String(sizeof(EEPROMdata.motorPosAtCCW)));
		Serial.println("[SETUP] motorPosAtCW " + String(sizeof(EEPROMdata.motorPosAtCW)));
		Serial.println("[SETUP] motorPosAtFullCCW " + String(sizeof(EEPROMdata.motorPosAtFullCCW)));
		Serial.println("[SETUP] motorPosAtFullCW " + String(sizeof(EEPROMdata.motorPosAtFullCW)));
		Serial.println("[SETUP] motorPosAtCCWset " + String(sizeof(EEPROMdata.motorPosAtCCWset)));
		Serial.println("[SETUP] motorPosAtCWset " + String(sizeof(EEPROMdata.motorPosAtCWset)));
		Serial.println("[SETUP] motorPos " + String(sizeof(EEPROMdata.motorPos)));
	}

	retrieveDataFromEEPROM();
	
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Found: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW?"True":"False")+", SW Version string: "+String(EEPROMdata.swVer));
		Serial.println("[SETUP] Found motor data: {CCW,CW,FCCW,FCW,CCWset,CWset,Pos}: {" + String(EEPROMdata.motorPosAtCCW) + "," + String(EEPROMdata.motorPosAtCW)+ "," + String(EEPROMdata.motorPosAtFullCCW)+ "," + String(EEPROMdata.motorPosAtFullCW )+ "," + String(EEPROMdata.motorPosAtCCWset)+ "," + String(EEPROMdata.motorPosAtCWset)+ "," + String(EEPROMdata.motorPos)+"}");  	
		Serial.println("[SETUP] Compiled init string: " + String(INITIALIZE) + ". Stored init string: " + String(EEPROMdata.initstr));
	}
	//always show the latest SW_VER
	strncpy(EEPROMdata.swVer, SW_VER, 11);
	if (DEBUG_MESSAGES)	
	{
		Serial.println("[SETUP] Actual swVER: " + String(EEPROMdata.swVer));
	}

	// change EEPROMdata in RAM
	if (String(INITIALIZE) != String(EEPROMdata.initstr))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Updating...");
		}
		//store specified personality data
		strncpy(EEPROMdata.initstr, INITIALIZE, 13);
		strncpy(EEPROMdata.namestr, CUSTOM_DEVICE_NAME, 41);
		strncpy(EEPROMdata.groupstr, CUSTOM_GROUP_NAME, 41);
		strncpy(EEPROMdata.typestr, CUSTOM_DEVICE_TYPE, 21);
		strncpy(EEPROMdata.note, CUSTOM_NOTE, 56);
		EEPROMdata.openIsCCW = OPEN_IS_CCW;
		//Just put defaults into motor data following Initialization
		EEPROMdata.motorPosAtCCW = -FULL_SWING + 3; 	
		EEPROMdata.motorPosAtCW = FULL_SWING - 3;		
		EEPROMdata.motorPosAtFullCCW = -FULL_SWING; 	
		EEPROMdata.motorPosAtFullCW = FULL_SWING;		
		EEPROMdata.motorPosAtCCWset = false;			
		EEPROMdata.motorPosAtCWset = false;			
		EEPROMdata.motorPos = 0; 	
		
		if (MASTER_NODE)
		{
			if (DEBUG_MESSAGES)
			{
				Serial.println("[SETUP] Setting this node as MASTER.");
			}
			EEPROMdata.master = true;
		}
		else
		{
			EEPROMdata.master = false;
		}
		
		storeDataToEEPROM();
	}
	else
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Nothing to update.");
		}
	}

	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Personality values are: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW?"True":"False")+", SW Version string: "+String(EEPROMdata.swVer));
		Serial.println("[SETUP] Motor data is: {CCW,CW,FCCW,FCW,CCWset,CWset,Pos}: {" + String(EEPROMdata.motorPosAtCCW) + "," + String(EEPROMdata.motorPosAtCW)+ "," + String(EEPROMdata.motorPosAtFullCCW)+ "," + String(EEPROMdata.motorPosAtFullCW)+ "," + String(EEPROMdata.motorPosAtCCWset)+ "," + String(EEPROMdata.motorPosAtCWset)+ "," + String(EEPROMdata.motorPos)+"}");  	

		Serial.println("Trying to retrieve again from EEPROM...");
		delay(500);
		retrieveDataFromEEPROM();
		delay(500);

		Serial.println("[SETUP] Personality values are: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW?"True":"False")+", SW Version string: "+String(EEPROMdata.swVer));
		Serial.println("[SETUP] Motor data is: {CCW,CW,FCCW,FCW,CCWset,CWset,Pos}: {" + String(EEPROMdata.motorPosAtCCW) + "," + String(EEPROMdata.motorPosAtCW)+ "," + String(EEPROMdata.motorPosAtFullCCW)+ "," + String(EEPROMdata.motorPosAtFullCW)+ "," + String(EEPROMdata.motorPosAtCCWset)+ "," + String(EEPROMdata.motorPosAtCWset)+ "," + String(EEPROMdata.motorPos)+"}");
	}

	//stepper motor setup
	stepper1.setMaxSpeed(MAX_SPEED);
	stepper1.setAcceleration(ACCELERATION);
	stepper1.setSpeed(0);
	stepper1.setCurrentPosition(EEPROMdata.motorPos);
	if((EEPROMdata.motorPosAtCCWset+EEPROMdata.motorPosAtCWset)==2){
		//if prior HW limits set state to man_idle to prevent unknown state effects
		deviceTrueState = man_idle;
	}

	// You can enable or disable the library at any moment
	// Disabling it will prevent the devices from being discovered and switched
	fauxmo.enable(false);

	if (!DEBUG_WiFiOFF)
	{
		// WiFi - see wifi-and-webserver.ino
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

//this is the main operating loop (MOL) that is repeatedly executed
void loop()
{
	//reset if commanded to by someone last loop
	if(resetOnNextLoop){
		//allow time for any commit to settle and webresponses to process before bailing
		delay(2000);
		resetController();
	}

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
			case unknown: //unknown state (bootup without stored HW limits)
				if (!CWlimSensorVal)
				{ //at CW limit
					EEPROMdata.motorPosAtFullCW = stepper1.currentPosition();
					EEPROMdata.motorPosAtCW = EEPROMdata.motorPosAtFullCW - 3; //back off of limit switch a bit
					EEPROMdata.motorPosAtCWset = true;
					EEPROMdata.motorPosAtCCW = EEPROMdata.motorPosAtCW - FULL_SWING;
					EEPROMdata.motorPosAtCCWset = false;
					idleActuator(closed); //make actuator idle
				}
				else if (!CCWlimSensorVal)
				{ //at CCW limit
					EEPROMdata.motorPosAtFullCCW = stepper1.currentPosition();
					EEPROMdata.motorPosAtCCW = EEPROMdata.motorPosAtFullCCW + 3; //back off of limit switch a bit
					EEPROMdata.motorPosAtCCWset = true;
					EEPROMdata.motorPosAtCW = EEPROMdata.motorPosAtCCW + FULL_SWING;
					EEPROMdata.motorPosAtCWset = false;
					idleActuator(opened); //make actuator idle
				}
				else
				{
					EEPROMdata.motorPosAtCWset = false;
					EEPROMdata.motorPosAtCCWset = false;
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
					Serial.printf("[MAIN loop cal] Found open (CCW) limit (%d). Going to calibration closing stage.\n", EEPROMdata.motorPosAtCCW);
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
					Serial.printf("[MAIN loop cal] Found close (CW) limit (%d). Calibration complete.\n", EEPROMdata.motorPosAtCW);
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
			EEPROMdata.motorPos = stepper1.currentPosition();
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
				Serial.printf("[MAIN] TrueState: %d, POS: %d, CW_LIM_SET? %s,CCW_LIM_SET? %s\n", deviceTrueState, stepper1.currentPosition(), EEPROMdata.motorPosAtCCWset ? "Y" : "N", EEPROMdata.motorPosAtCWset ? "Y" : "N");
				Serial.printf("[MAIN] Manual Switch Positions CCWman: %d; CWman: %d; SELman: %d\n", CCWmanSensorVal, CWmanSensorVal, SELmanSensorVal);
			}
		}
	}
}

//This function takes messages from some remote address (if from another node)
//that are of maximum lenght MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web if this
//is the MASTER node running the webserver 
void executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
	String cmd = String(inputMsg);
	cmd.toLowerCase();
	if (cmd.startsWith("open"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded OPEN");
		}
		executeState(true); //WiFi "true" is open or going CCW to stop
	}
	else if (cmd.startsWith("close"))
	{
		Serial.println("[executeCommands] Commanded CLOSE");
		executeState(false); //WiFi "false" is close or going CW to stop
	}
	else if (cmd.startsWith("goto"))
	{ 
		Serial.println("[executeCommands] Commanded GOTO (" + cmd + ")");
		executeGoto(cmd); //WiFi "false" is close or going CW to stop
	}
	else if (cmd.startsWith("hi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Hello!");
		}
	}
	else if (cmd.startsWith("reset"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET");
		}
		resetOnNextLoop=true;
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
	else if (cmd.startsWith("reset_wifi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET_WIFI");
		}
		//TODO - make this wifi reset work --  this doesn;t do it.
		WiFi.disconnect(true);
		delay(2000);
		ESP.reset();
	}
	else
	{
		Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);
	}
}

//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
	int newPercentOpen = whichWayGoto(cmd.substring(4).toInt()); //STRIP OFF GOTO and correct for openisCCCW
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeGoto] Going to percent open: %d\n", newPercentOpen);
	}
	deviceTrueState = openPercentActuator(newPercentOpen);
}
//execute a WiFi received state change
void executeState(bool state)
{
	bool correctedState = whichWay(state);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (correctedState)
	{
		deviceTrueState = openPercentActuator(100);
	}
	else
	{
		deviceTrueState = openPercentActuator(0);
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
		EEPROMdata.motorPosAtFullCW = curPos;
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
		EEPROMdata.motorPosAtFullCCW = curPos;
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
			resetOnNextLoop = true;
		}
		break;
	}
	lastManCommand = SEL;
}

void setCWLimit(long newLimit)
{
	EEPROMdata.motorPosAtCW = newLimit;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[setCWLimit] New Limit: %d \n", newLimit);
	}
	EEPROMdata.motorPosAtCWset = true;
	if (EEPROMdata.motorPosAtCCW >= EEPROMdata.motorPosAtCW)
	{ //if limits are reversed, update the CCW limit
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[setCWLimit] LIMITS REVERSED, updating CCW Limit: %d ", newLimit - 1);
		}
		EEPROMdata.motorPosAtCCW = EEPROMdata.motorPosAtCW - 1;
		EEPROMdata.motorPosAtCCWset = true;
	}
}

void setCCWLimit(long newLimit)
{
	EEPROMdata.motorPosAtCCW = newLimit;
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[setCCWLimit] New Limit: %d \n", newLimit);
	}
	EEPROMdata.motorPosAtCCWset = true;
	if (EEPROMdata.motorPosAtCW <= EEPROMdata.motorPosAtCCW)
	{ //if limits are reversed, update the CW limit
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[setCCWLimit] LIMITS REVERSED, updating CW Limit: %d \n", newLimit + 1);
		}
		EEPROMdata.motorPosAtCW = EEPROMdata.motorPosAtCCW + 1;
		EEPROMdata.motorPosAtCCWset = true;
	}
}

//function used to do a normal WiFi or calibration opening of the actuator
void normalOpening()
{
	if (!digitalRead(SWpinLimitCCW))
	{
		//reached limit - disable everything and reset motor to idle
		EEPROMdata.motorPosAtFullCCW = stepper1.currentPosition();
		EEPROMdata.motorPosAtCCW = EEPROMdata.motorPosAtFullCCW + 3; //back off of limit switch a bit
		EEPROMdata.motorPosAtCCWset = true;

		if (!EEPROMdata.motorPosAtCWset)
		{ //if opposite position not set, set guess at full swing
			EEPROMdata.motorPosAtCW = EEPROMdata.motorPosAtCCW + FULL_SWING;
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
		EEPROMdata.motorPosAtFullCW = stepper1.currentPosition();
		EEPROMdata.motorPosAtCW = EEPROMdata.motorPosAtFullCW - 3; //back off of limit switch a bit
		EEPROMdata.motorPosAtCWset = true;

		if (!EEPROMdata.motorPosAtCCWset)
		{ //if opposite position not set, set guess at full swing
			EEPROMdata.motorPosAtCCW = EEPROMdata.motorPosAtCW - FULL_SWING;
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
//this function replaces open with close if 
//CW is defined as open by openIsCCW
bool whichWay(bool in){
	bool ret=in;
	if(EEPROMdata.openIsCCW==true){
		ret = !ret;
	}
	return ret;
}
//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as 
//open by openIsCCW
int whichWayGoto(int in){
	int ret=in;
	if(EEPROMdata.openIsCCW==false){
		ret = 100-ret;
	}
	return ret;
}
//put the actuator/stepper-motor in an idle state, store position in EEPROM, 
//and annonuce the final position to the MASTER
trueState idleActuator(trueState idleState)
{
	stepper1.stop();
	stepper1.setSpeed(0);
	stepper1.disableOutputs();
	deviceTrueState = idleState;
	EEPROMdata.motorPos = int(stepper1.currentPosition());
	storeDataToEEPROM();
	UDPpollReply(masterIP); //tell the Master Node the new info
	return idleState;
}
//TODO - implement this for CCW is open, and make the opposite for CW is open
// explain things were designed from the perspective of CCW is open
trueState openPercentActuator(int percent)
{
	trueState newState;
	int newMotorPos = (int)((EEPROMdata.motorPosAtCW - EEPROMdata.motorPosAtCCW) * percent / 100 + EEPROMdata.motorPosAtCCW);
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
				Serial.printf("[openPercentActuator] Moving CCW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), EEPROMdata.motorPosAtCCW);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(-START_SPEED);
		}
		else
		{
			EEPROMdata.motorPosAtFullCCW = stepper1.currentPosition();
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
				Serial.printf("[openPercentActuator] Moving CW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), EEPROMdata.motorPosAtCW);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(START_SPEED);
		}
		else
		{
			EEPROMdata.motorPosAtFullCW = stepper1.currentPosition();
			newState = idleActuator(closed);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
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

//put the controller in a unknown state ready for calibration
void resetController()
{
	//TODO - save the motor variables and calibration ranges first.
	//TODO - if wifireset is required also do that here

	ESP.restart();
	//asm volatile ("  jmp 0");  
	//ESP.reset();
}