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
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <fauxmoESP.h>
#include <AccelStepper.h>
#include <EEPROM.h>
#include <StreamString.h>

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
		Serial.println("[SETUP] motorPosAtCCWset " + String(sizeof(EEPROMdata.motorPosAtCCWset)));
		Serial.println("[SETUP] motorPosAtCWset " + String(sizeof(EEPROMdata.motorPosAtCWset)));
		Serial.println("[SETUP] motorPos " + String(sizeof(EEPROMdata.motorPos)));
		Serial.println("[SETUP] range " + String(sizeof(EEPROMdata.range)));
		Serial.println("[SETUP] swapLimSW " + String(sizeof(EEPROMdata.swapLimSW)));
		Serial.println("[SETUP] timeOut " + String(sizeof(EEPROMdata.timeOut)));
		Serial.println("[SETUP] deviceTimedOut " + String(sizeof(EEPROMdata.deviceTimedOut)));
	}

	retrieveDataFromEEPROM();
	
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Found: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW?"True":"False") + ", SwapLimSW: "+String(EEPROMdata.swapLimSW?"True":"False") + ", SW Version string: "+String(EEPROMdata.swVer) + ", Motor Timeout "+String(EEPROMdata.timeOut)+ ", Device Timedout "+String(EEPROMdata.deviceTimedOut));
		Serial.println("[SETUP] Found motor data: {CCWset,CWset,Pos,range}: {" + String(EEPROMdata.motorPosAtCCWset)+ "," + String(EEPROMdata.motorPosAtCWset)+ "," + String(EEPROMdata.motorPos)+ "," + String(EEPROMdata.range)+"}");  	
		Serial.println("[SETUP] Compiled init string: " + String(INITIALIZE) + ". Stored init string: " + String(EEPROMdata.initstr));
	}

	// change EEPROMdata in RAM
	if (String(INITIALIZE) != String(EEPROMdata.initstr))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Updating...");
		}
		//store specified personality data
		strncpy(EEPROMdata.swVer, SW_VER, 11);
		strncpy(EEPROMdata.initstr, INITIALIZE, 13);
		strncpy(EEPROMdata.namestr, CUSTOM_DEVICE_NAME, 41);
		strncpy(EEPROMdata.groupstr, CUSTOM_GROUP_NAME, 41);
		strncpy(EEPROMdata.typestr, CUSTOM_DEVICE_TYPE, 21);
		strncpy(EEPROMdata.note, CUSTOM_NOTE, 56);
		EEPROMdata.openIsCCW = OPEN_IS_CCW;
		EEPROMdata.swapLimSW = SWAP_LIM_SW;
		EEPROMdata.timeOut = MOTOR_TIMEOUT;

		//Just put defaults into motor data following Initialization
		EEPROMdata.motorPosAtCCWset = false;	
		EEPROMdata.motorPosAtCWset = false;			
		EEPROMdata.motorPos = 0;
		EEPROMdata.range = FULL_SWING; 	
		EEPROMdata.deviceTimedOut = false;	

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
		//always show the latest SW_VER
		strncpy(EEPROMdata.swVer, SW_VER, 11);
		if (DEBUG_MESSAGES)	
		{
			Serial.println("[SETUP] Actual swVER: " + String(EEPROMdata.swVer));
		}
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Nothing else to update.");
		}
		storeDataToEEPROM();
	}

	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Personality values are: Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", OpenIsCCW: "+String(EEPROMdata.openIsCCW?"True":"False")+", SwapLimSW: "+String(EEPROMdata.swapLimSW?"True":"False") + ", SW Version string: "+String(EEPROMdata.swVer)+ ", Motor Timeout "+String(EEPROMdata.timeOut)+ ", Device Timedout "+String(EEPROMdata.deviceTimedOut));
		Serial.println("[SETUP] Motor data is: {CCWset,CWset,Pos,range}: {" + String(EEPROMdata.motorPosAtCCWset)+ "," + String(EEPROMdata.motorPosAtCWset)+ "," + String(EEPROMdata.motorPos) + "," + String(EEPROMdata.range) + "}");  	

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
		if(FAUXMO_ENABLED){
			fauxmo.enable(true);
			// Add virtual device
			fauxmo.addDevice(EEPROMdata.namestr);
		}
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

	if(FAUXMO_ENABLED){
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
	}
	
	//announce master if this is the mastr node
	//otherwise let the master know you're here
	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	}else{
		UDPbroadcast();
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
			if (DEBUG_MESSAGES){ Serial.println("[MAIN] Announcing master"); }
			UDPannounceMaster();
		}
		static unsigned long lastNodeCulling = millis();
		if (millis() - lastNodeCulling > 600000) //cull dead nodes from the list every 10 minutes
		{
			lastNodeCulling = millis();
			if (DEBUG_MESSAGES){ Serial.println("[MAIN] Removing Dead Nodes"); }
			cullDeadNodes();
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
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		 CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		 CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		 CCWlimSensorVal = digitalRead(SWpinLimitCW);
		 CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}
	//STATE MACHINE:
	//First -- check for a calibration in progress; if so - finish calibration.
	//second - go through all the other actions based on deviceTrueState.

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
		case opening: //opening not at limit (really just moving CCW)
			deviceTrueState = moveCCW();
			break;
		case closing: //closing not at limit (really just moving CW)
			deviceTrueState = moveCW();
			break;
		case unknown: //unknown state (bootup without stored HW limits)
			if (!CWlimSensorVal)
			{ //at CW limit
				EEPROMdata.range = stepper1.currentPosition();
				EEPROMdata.motorPosAtCWset = true;
				EEPROMdata.motorPosAtCCWset = false;
				//make actuator idle
				if(EEPROMdata.openIsCCW){
					idleActuator(closed); 
				}else{
					idleActuator(opened);
				}
			}
			else if (!CCWlimSensorVal)
			{ //at CCW limit
				stepper1.setCurrentPosition(0);
				EEPROMdata.motorPosAtCCWset = true;
				EEPROMdata.range = FULL_SWING;
				EEPROMdata.motorPosAtCWset = false;
					//make actuator idle
				if(EEPROMdata.openIsCCW){
					idleActuator(opened); 
				}else{
					idleActuator(closed);
				}
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
		EEPROMdata.range = FULL_SWING;		
		EEPROMdata.motorPosAtCCWset = false;			
		EEPROMdata.motorPosAtCWset = false;	
		deviceTrueState = moveCCW();
		if (deviceTrueState == opened)
		{ //Done with stage 1 go onto closing stage
			deviceCalStage = closingCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN loop cal] Found open (CCW) limit (0 by def'n). Set motorPos to 0. Going to calibration closing stage.\n");
			}
			executeState(false);
		}
		break;
	case closingCal: //calibration in second stage
		moveCW();
		if (deviceTrueState == closed)
		{ //Done with stage 2 - done!
			deviceCalStage = doneCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN loop cal] Found close (CW) limit (%d). Calibration complete.\n", EEPROMdata.range);
			}
		}
		break;
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
		executeState(true); //WiFi "true" is open (going CCW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("close"))
	{
		Serial.println("[executeCommands] Commanded CLOSE");
		executeState(false); //WiFi "false" is close (going CW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("stop"))
	{
		Serial.println("[executeCommands] Commanded STOP");
		executeStop();
	}
	else if (cmd.startsWith("goto"))
	{ 
		Serial.println("[executeCommands] Commanded GOTO (" + cmd + ")");
		executeGoto(cmd); 
	}
	else if (cmd.startsWith("hi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Hello!");
		}
	}
	else if (cmd.startsWith("reset_wifi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET_WIFI");
		}
		
		WiFiManager.resetSettings();
		resetOnNextLoop = true;
		delay(2000);
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
	{
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
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded ANYFISHYDEV_THERE");
		}
		UDPpollReply(remote);
	}
	else if (cmd.startsWith("poll_net"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_NET");
		}
		UDPbroadcast();
	}
	else if (cmd.startsWith("poll_response"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_RESPONSE");
		}
		UDPparsePollResponse(String(inputMsg), remote); //want the original case for this
	}
	else if (cmd.startsWith("fishydiymaster"))
	{
		if (DEBUG_UDP_MESSAGES)
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

//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
	//reset the motor timeout counter
	motorRunTime = millis();
	EEPROMdata.deviceTimedOut = false;
	//ensure motor output is enabled
	stepper1.enableOutputs();
	int newPercentOpen = whichWayGoto(cmd.substring(4).toInt()); //STRIP OFF GOTO and correct for openisCCCW
	if(newPercentOpen == 100){
		targetPos=-1;
		deviceTrueState = moveCCW();
	}else if(newPercentOpen == 0){
		targetPos=-1;
		deviceTrueState = moveCW();
	}else{ //targetting mid position
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[executeGoto] Going to percent open: %d\n", newPercentOpen);
		}
		deviceTrueState = openPercentActuator(newPercentOpen);
	}
}
//stops acutator where it is
void executeStop()
{
	
	if (DEBUG_MESSAGES)
	{
		Serial.println("[executeStop] Stopping actuator");
	}
	deviceTrueState = man_idle;
	idleActuator(deviceTrueState);
}
//execute a WiFi received state change
// ON equates to open
// OFF equates to close
void executeState(bool state)
{
	//reset the motor timeout counter
	motorRunTime = millis();
	EEPROMdata.deviceTimedOut = false;
	//ensure motor output is enabled
	stepper1.enableOutputs();
	//determine correct direction based on OpenisCCW and correct the state
	bool correctedState = whichWay(state);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (correctedState)
	{
		targetPos=-1;
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeState] Transition to moving CCW...");
		}
		deviceTrueState = moveCCW();	
	}
	else
	{
		targetPos=-1;
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeState] Transition to moving CW...");
		}
		deviceTrueState = moveCW();
	}
}

//function used to do a normal WiFi or calibration opening of the actuator
// - this moves at constant speed to HW limits
trueState moveCCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}
	trueState newState;
	if (CCWlimSensorVal==0)
	{
		//reached limit - disable everything and reset motor to idle
		// -set stepper pos = 0;  (by definition fullCCW is 0) and
		// -update flag indicating the HW lim sw was found
		stepper1.setCurrentPosition(0);
		EEPROMdata.motorPosAtCCWset = true;
		newState = idleActuator(opened); //make actuator idle
		if (DEBUG_MESSAGES)
		{

			Serial.printf("[moveCCW] Reached CCW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}else{ //keep going if still have distance to travel
		newState = opening;
		
		if(targetPos>-1){ 
			//if true then moving to a given position
			stepper1.run();
			//see if done or if timedout
			if((stepper1.currentPosition()==targetPos)){
				newState = idleActuator(man_idle);
			}
			if((millis()-motorRunTime)>EEPROMdata.timeOut*1000){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}else{
			if (stepper1.speed() != -MAX_SPEED)
			{
				stepper1.setSpeed(-MAX_SPEED);
			}
			stepper1.runSpeed();
			//see if timedout
			if(((millis()-motorRunTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}

//function used to do a normal WiFi or calibration closing of the actuator
trueState moveCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}
	
	trueState newState;
	if (CWlimSensorVal==0)
	{
		//reached limit - disable everything and reset motor to idle
		// -set range to current stepper pos and
		// -update flag indicating the HW lim sw was found
		EEPROMdata.range = stepper1.currentPosition();
		EEPROMdata.motorPosAtCWset = true;

		newState = idleActuator(closed); //make actuator idle
		if (DEBUG_MESSAGES)
		{

			Serial.printf("[moveCW] Reached CW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}else{ //keep going if still have distance to travel
		newState = closing;
		
		if(targetPos>-1){ 
			//if true then moving to a given position using acceleration
			stepper1.run();
			//see if done or if timedout
			if((stepper1.currentPosition()==targetPos)){
				newState = idleActuator(man_idle);
			}
			if((millis()-motorRunTime)>EEPROMdata.timeOut*1000){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}else{
			if (stepper1.speed() != MAX_SPEED)
			{
				stepper1.setSpeed(MAX_SPEED);
			}
			stepper1.runSpeed();
			//see if timedout
			if(((millis()-motorRunTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}
//this function swaps the state (makes open command go CW) if CW is defined as open by openIsCCW=false
bool whichWay(bool in){
	bool ret=in;
	if(EEPROMdata.openIsCCW==false){
		ret = !ret;
	}
	return ret;
}

//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as 
//open by openIsCCW
int whichWayGoto(int in){
	int ret=in;
	if(EEPROMdata.openIsCCW==true){
		ret = 100-ret;
	}
	return ret;
}

//put the actuator/stepper-motor in an idle state, store position in EEPROM, 
//and annonuce the final position to the MASTER
trueState idleActuator(trueState idleState)
{
	targetPos = -1;
	stepper1.stop();
	stepper1.setSpeed(0);
	stepper1.disableOutputs();
	deviceTrueState = idleState;
	EEPROMdata.motorPos = int(stepper1.currentPosition());
	storeDataToEEPROM();
	UDPpollReply(masterIP); //tell the Master Node the new info
	return idleState;
}

//this detemines which way to goto to get to a commanded position
//sets targetPos and commands open or close
//motor CCW limit = 0, motor CW limit = range, if no target set targetPos = -1
trueState openPercentActuator(int percent)
{
	trueState newState;
	int newMotorPos = (int)(EEPROMdata.range * percent / 100);
	targetPos = newMotorPos;
	
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}

	if (DEBUG_MESSAGES)
	{
		Serial.printf("[openPercentActuator] Going to %d percent open. New Pos: %d\n", percent, newMotorPos);
	}
	if (newMotorPos < stepper1.currentPosition())
	{ //NEED TO MOVE CCW
		if (CCWlimSensorVal == 1)
		{ //switch is open; not at CCW limit
			newState = opening;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CCW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), newMotorPos);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(-START_SPEED);
		}
		else
		{
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
	
		if (CWlimSensorVal == 1)
		{ //switch is open; not at CW limit
			newState = closing;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), newMotorPos);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(START_SPEED);
		}
		else
		{
			EEPROMdata.range = stepper1.currentPosition();
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

	pinMode(LED_BUILTIN, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on by making the voltage LOW
		delay(100);						 // Wait for a second
		digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
		delay(200);						 // Wait for two seconds
	}
	pinMode(LED_BUILTIN, INPUT_PULLUP);
}

void slowBlinks(int numBlinks)
{

	pinMode(LED_BUILTIN, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on by making the voltage LOW
		delay(1000);					 // Wait for a second
		digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
		delay(1000);					 // Wait for two seconds
	}
	pinMode(LED_BUILTIN, INPUT_PULLUP);
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
	int relPos = device.motorPos;
	//int range = device.motorPosAtCW - device.motorPosAtCCW;
	int answer;
	if (device.range > 0)
	{
		answer = (int)(100 * relPos / device.range);
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
	ESP.restart();
}