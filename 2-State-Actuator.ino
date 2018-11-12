//THIS FILE HAS CUSTOM GLOBAL VARIABLES, EXTERNAL, AND INTERNAL FUNCTIONS FOR 2-STATE-ACTUATORS

//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - EXTERNAL - For 2-state-actuator 
//COMMON EXT functions are:
// void {operateDevice(),deviceSetup(),executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)),executeState(bool state),UDPparseConfigResponse(String responseIn, IPAddress remote)}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
String getStatusString(){
   	String statusStr = "Current Position:";
   	int pos;
   	if(EEPROMdeviceData.range==0) {
		statusStr = statusStr + "0% Open";
		if(DEBUG_MESSAGES){Serial.println("[getStatusString] Range of 0 found.");}
	}
	else{
	   	pos = round(((float)EEPROMdeviceData.motorPos*100.0)/(float)EEPROMdeviceData.range);
	   	if(EEPROMdeviceData.openIsCCW) pos = 100-pos;
        statusStr = statusStr + String(pos) + "% Open";
	}
   	return statusStr;
	if(DEBUG_MESSAGES){Serial.println("[getStatusString] Status:" + String(statusStr));}
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//encode compiled (settings) device data into the char[256] for storage in EEPROMdata for new devices
void initializeDeviceCustomData(){
	    String builder = String("openIsCCW=") + String(OPEN_IS_CCW?"1":"0") + String("&swapLimSW=") + String(SWAP_LIM_SW?"1":"0") + String("&motorPosAtCCWset=0&motorPosAtCWset=0&motorPos=0&range=") + String(FULL_SWING);
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);
		extractDeviceCustomData();
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//extract custom device data from char[256] in EEPROMdata and put it into the device specific struct
void extractDeviceCustomData(){
	//example EEPROMdata.deviceCustomData = "openIsCCW=1&swapLimSW=0&motorPosAtCCWset=0&motorPosAtCWset=0&motorPos=2340&range=8889";
    char *strings[12];
    char *ptr = NULL;
    byte index = 0;
    
	if(DEBUG_MESSAGES){Serial.println("[extractDeviceCustomData]" + String(EEPROMdata.deviceCustomData));}
    
	ptr = strtok(EEPROMdata.deviceCustomData, "=&");  // takes a list of delimiters and points to the first token
    while(ptr != NULL)
    {
        strings[index] = ptr;
        index++;
        ptr = strtok(NULL, "=&");  // goto the next token
    }
    
    if(DEBUG_MESSAGES){for(int n = 0; n < index; n++){Serial.print(n);Serial.print(") ");Serial.println(strings[n]);}}

	//names are even (0,2,4..), data is odd(1,3,5..)
	EEPROMdeviceData.openIsCCW = (String(strings[1])=="1")?true:false;		
	EEPROMdeviceData.swapLimSW = (String(strings[3])=="1")?true:false;
	EEPROMdeviceData.motorPosAtCCWset = (String(strings[5])=="1")?true:false;	
	EEPROMdeviceData.motorPosAtCWset = (String(strings[7])=="1")?true:false;	
	EEPROMdeviceData.motorPos = atoi(strings[9]); 						
	EEPROMdeviceData.range = atoi(strings[11]); 

    if(DEBUG_MESSAGES){
		String builder = "openIsCCW=" + String(EEPROMdeviceData.openIsCCW?"1":"0") + "&swapLimSW=" + String(EEPROMdeviceData.swapLimSW?"1":"0") + "&motorPosAtCCWset=" + String(EEPROMdeviceData.motorPosAtCCWset?"1":"0") + "&motorPosAtCWset=" + String(EEPROMdeviceData.motorPosAtCWset?"1":"0") + "&motorPos=" + String(EEPROMdeviceData.motorPos) + "&range=" + String(EEPROMdeviceData.range);
		Serial.println(builder);

		showEEPROMdevicePersonalityData();
	}

}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//encode dynamic device data into the char[256] for storage in EEPROMdata
void encodeDeviceCustomData(){
	    String builder = "openIsCCW=" + String(EEPROMdeviceData.openIsCCW?"1":"0") + "&swapLimSW=" + String(EEPROMdeviceData.swapLimSW?"1":"0") + "&motorPosAtCCWset=" + String(EEPROMdeviceData.motorPosAtCCWset?"1":"0") + "&motorPosAtCWset=" + String(EEPROMdeviceData.motorPosAtCWset?"1":"0") + "&motorPos=" + String(EEPROMdeviceData.motorPos) + "&range=" + String(EEPROMdeviceData.range);
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//encode dynamic device data into the char[256] for storage in EEPROMdata
void showEEPROMdevicePersonalityData(){
	if(DEBUG_MESSAGES){
		Serial.println("[SETUP-device] OpenIsCCW: "+String(EEPROMdeviceData.openIsCCW?"True":"False") + ", SwapLimSW: "+String(EEPROMdeviceData.swapLimSW?"True":"False"));
		Serial.println("[SETUP-device] Found motor data: {CCWset,CWset,Pos,range}: {" + String(EEPROMdeviceData.motorPosAtCCWset)+ "," + String(EEPROMdeviceData.motorPosAtCWset)+ "," + String(EEPROMdeviceData.motorPos)+ "," + String(EEPROMdeviceData.range)+"}");  	
	}
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//return if the device is calibrated (the common function checks for deviceTimedOut)
bool isCustomDeviceReady(){
	//if both limit switch positions have been stored the device is calibrated
	if (EEPROMdeviceData.motorPosAtCCWset && EEPROMdeviceData.motorPosAtCWset)
	{
		return true;
	}else{
		return false;
	}
}

String getDeviceSpecificJSON(){
	String temp;
	temp = "{\"fishyDevices\":[";
	/* 
	put this 2-State-Actuator data in a string.
	Note - this is string will be parsed by scripts in the custom device webpage (webresource.h) - if adding data elements all these may need updating.  
	This function sends data as follows (keep this list updated):
	{ip,motorPosAtCWset,motorPosAtCCWset,isMaster,motorPos,name,openIsCCW,port,group,note,swVer,devType,initStamp,range,timeOut,deviceTimedOut,swapLimSW}
	*/
	temp += "{\"ip\":\"" + WiFi.localIP().toString() +
			"\",\"motorPosAtCWset\":\"" + String(EEPROMdeviceData.motorPosAtCWset ? "true" : "false") +
			"\",\"motorPosAtCCWset\":\"" + String(EEPROMdeviceData.motorPosAtCCWset ? "true" : "false") +
			"\",\"isMaster\":\"" + String(EEPROMdata.master ? "true" : "false") + "\",\"motorPos\":\"" + String(int(stepper1.currentPosition())) +
			"\",\"deviceName\":\"" + String(EEPROMdata.namestr) + "\",\"openIsCCW\":\"" + String(EEPROMdeviceData.openIsCCW ? "true" : "false") + "\",\"group\":\"" + String(EEPROMdata.groupstr) + 
			"\",\"note\":\"" + String(EEPROMdata.note) + "\",\"swVer\":\"" + String(EEPROMdata.swVer) + 
			"\",\"devType\":\"" + String(EEPROMdata.typestr) + "\",\"initStamp\":\"" + String(EEPROMdata.initstr) + 
			"\",\"range\":\"" + String(EEPROMdeviceData.range) + "\",\"timeOut\":\"" + String(EEPROMdata.timeOut) + 
			"\",\"deviceTimedOut\":\"" + String(EEPROMdata.deviceTimedOut ? "true" : "false") + 
			"\",\"swapLimSW\":\"" + String(EEPROMdeviceData.swapLimSW ? "true" : "false") + "\"}";

	temp += "]}";
	return temp;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void operateDevice()
{
	operateLimitSwitchActuator();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void deviceSetup()
{
	pinSetup();
	motorSetup();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//This function takes messages from some remote address (if from another node)
//that are of maximum lenght MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//REQD COMMANDS: {anyfishydev_there,fishydiymaster,poll_net,poll_response,reset_wifi,reset} -> needed to be a fishyDevice on the network
//VALID DEVICE COMMANDS: 
// (open,close,stop,gotoXXX,{hi.hello},calibrate,config[;semi-colon-separated-parameters=values])
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
		updateClients("Open Received", true);
		executeState(true); //WiFi "true" is open (going CCW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("close"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CLOSE");
		}
		updateClients("Close Received", true);
		executeState(false); //WiFi "false" is close (going CW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("stop"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded STOP");
		}
		updateClients("Stop Received", true);
		executeStop();
	}
	else if (cmd.startsWith("goto"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded GOTO (" + cmd + ")");
		}
		updateClients("Goto (" + cmd + ") Received", true);
		executeGoto(cmd); 
	}
	else if (cmd.startsWith("hi")||cmd.startsWith("hello"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Hello! My IP is:");
			Serial.println(WiFi.localIP().toString());
			Serial.println("[executeCommands] Here is my personality info (ignore the [Setup] tag):");
			showEEPROMPersonalityData();
		}
	}
	else if (cmd.startsWith("reset_wifi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET_WIFI");
		}
		updateClients("Resetting WiFi",true);
		AsyncWiFiManager WiFiManager(&httpServer,&dns);
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
		updateClients("Rebooting Device",true);
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
		updateClients("Calibrating", true);
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
		updateClients("Settings Updated.", true);
	}
	else
	{
		Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);
	}
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//configuration response from web/udp - this preforms a configuration update to the device and stores it in EEPROM
void UDPparseConfigResponse(String responseIn, IPAddress remote){
	String response = responseIn.substring(7); //strip off "CONFIG"
	int strStrt, strStop;

	//parseString in this order: {openIsCCW, isMaster, devName, groupName, devType, note}
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] Got this: " + responseIn);
	}	
	//openIsCCW
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	EEPROMdeviceData.openIsCCW = (response.substring(strStrt, strStop) == "false") ? false : true;
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] openIsCCW: ");
		Serial.println(EEPROMdeviceData.openIsCCW ? "true" : "false");
	}
	//isMaster
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
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
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] devName: " + String(EEPROMdata.namestr));
	}
	//groupName
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.groupstr, response.substring(strStrt, strStop).c_str(), 41);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] groupName: " + String(EEPROMdata.groupstr));
	}
	//devType
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.typestr, response.substring(strStrt, strStop).c_str(), 21);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] devType: " + String(EEPROMdata.typestr));
	}
	//note
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.note, response.substring(strStrt, strStop).c_str(), 56);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] note: " + String(EEPROMdata.note));
	}
	//swapLimSW
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	EEPROMdeviceData.swapLimSW = (response.substring(strStrt, strStop) == "false") ? false : true;
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] swapLimSW: ");
		Serial.println(EEPROMdeviceData.swapLimSW ? "true" : "false");
	}
	//timeOut
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	EEPROMdata.timeOut = response.substring(strStrt, strStop).toInt();
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] timeOut: " + String(EEPROMdata.timeOut));
	}
	//inform the master of new settings
	UDPpollReply(masterIP);
	//save them to EEPROM
	storeDataToEEPROM();
}


//CUSTOM DEVICE FUNCTION - EXTERNAL (FOR ALL FAUXMO ENABLED DEVICES)
// execute a WiFi received state change
// ON equates to open
// OFF equates to close
void executeState(bool state)
{
	//reset the motor timeout counter
	deviceResponseTime = millis();
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
//=====================================================================================================================


//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
//set pin modes
void pinSetup(){
	//set switch pins to use internal pull_up resistor
	pinMode(SWpinLimitCW, INPUT_PULLUP);
	pinMode(SWpinLimitCCW, INPUT_PULLUP);
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
//set up motor parameters
void motorSetup(){
	//stepper motor setup
	stepper1.setMaxSpeed(MAX_SPEED);
	stepper1.setAcceleration(ACCELERATION);
	stepper1.setSpeed(0);
	stepper1.setCurrentPosition(EEPROMdeviceData.motorPos);
	if((EEPROMdeviceData.motorPosAtCCWset+EEPROMdeviceData.motorPosAtCWset)==2){
		//if prior HW limits set state to man_idle to prevent unknown state effects
		deviceTrueState = man_idle;
	}
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
void operateLimitSwitchActuator(){
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdeviceData.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
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
			updateClients("Moving CCW");
			break;
		case closing: //closing not at limit (really just moving CW)
			deviceTrueState = moveCW();
			updateClients("Moving CW");
			break;
		case unknown: //unknown state (bootup without stored HW limits)
			if (!CWlimSensorVal)
			{ //at CW limit
				EEPROMdeviceData.range = stepper1.currentPosition();
				EEPROMdeviceData.motorPosAtCWset = true;
				EEPROMdeviceData.motorPosAtCCWset = false;
				//make actuator idle
				if(EEPROMdeviceData.openIsCCW){
					idleActuator(closed); 
				}else{
					idleActuator(opened);
				}
			}
			else if (!CCWlimSensorVal)
			{ //at CCW limit
				stepper1.setCurrentPosition(0);
				EEPROMdeviceData.motorPosAtCCWset = true;
				EEPROMdeviceData.range = FULL_SWING;
				EEPROMdeviceData.motorPosAtCWset = false;
					//make actuator idle
				if(EEPROMdeviceData.openIsCCW){
					idleActuator(opened); 
				}else{
					idleActuator(closed);
				}
			}
			else
			{
				EEPROMdeviceData.motorPosAtCWset = false;
				EEPROMdeviceData.motorPosAtCCWset = false;
			}
			break;
		}
		break;
	case openingCal: //calibration in first stage	
		EEPROMdeviceData.range = FULL_SWING;		
		EEPROMdeviceData.motorPosAtCCWset = false;			
		EEPROMdeviceData.motorPosAtCWset = false;	
		updateClients("Calibrating - CCW");
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
		updateClients("Calibrating - CW");
		moveCW();
		if (deviceTrueState == closed)
		{ //Done with stage 2 - done!
			deviceCalStage = doneCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN loop cal] Found close (CW) limit (%d). Calibration complete.\n", EEPROMdeviceData.range);
			}
		}
		break;
	}
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
	//reset the motor timeout counter
	deviceResponseTime = millis();
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
//CUSTOM DEVICE FUNCTION - INTERNAL
//stops acutator where it is
void executeStop()
{
	//reset the motor timeout counter
	deviceResponseTime = millis();
	EEPROMdata.deviceTimedOut = false;
	if (DEBUG_MESSAGES)
	{
		Serial.println("[executeStop] Stopping actuator");
	}
	deviceTrueState = man_idle;
	deviceCalStage = doneCal;
	idleActuator(deviceTrueState);
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//function used to do a normal WiFi or calibration opening of the actuator
// - this moves at constant speed to HW limits
trueState moveCCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdeviceData.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
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
		EEPROMdeviceData.motorPosAtCCWset = true;
		newState = idleActuator(opened); //make actuator idle
		if (DEBUG_MESSAGES)	{Serial.printf("[moveCCW] Reached CCW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);}
	}else{ //keep going if still have distance to travel
		newState = opening;
		
		if(targetPos>-1){ 
			//if true then moving to a given position
			stepper1.run();
			//see if done or if timedout
			if((stepper1.currentPosition()==targetPos)){
				newState = idleActuator(man_idle);
			}
			if((millis()-deviceResponseTime)>EEPROMdata.timeOut*1000){
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
			if(((millis()-deviceResponseTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//function used to do a normal WiFi or calibration closing of the actuator
trueState moveCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdeviceData.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
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
		EEPROMdeviceData.range = stepper1.currentPosition();
		EEPROMdeviceData.motorPosAtCWset = true;

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
			if((millis()-deviceResponseTime)>EEPROMdata.timeOut*1000){
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
			if(((millis()-deviceResponseTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this function swaps the state (makes open command go CW) if CW is defined as open by openIsCCW=false
bool whichWay(bool in){
	bool ret=in;
	if(EEPROMdeviceData.openIsCCW==false){
		ret = !ret;
	}
	return ret;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as 
//open by openIsCCW
int whichWayGoto(int in){
	int ret=in;
	if(EEPROMdeviceData.openIsCCW==true){
		ret = 100-ret;
	}
	return ret;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//put the actuator/stepper-motor in an idle state, store position in EEPROM, 
//and annonuce the final position to the MASTER
trueState idleActuator(trueState idleState)
{
	targetPos = -1;
	stepper1.stop();
	stepper1.setSpeed(0);
	stepper1.disableOutputs();
	deviceTrueState = idleState;
	EEPROMdeviceData.motorPos = int(stepper1.currentPosition());
	storeDataToEEPROM();
	UDPpollReply(masterIP); //tell the Master Node the new info
	updateClients("Stopped.", true);
	return idleState;

	//TODO - fix this - add update to ensure the websocket coonnected panels are refreshed when stopped
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this detemines which way to goto to get to a commanded position
//sets targetPos and commands open or close
//motor CCW limit = 0, motor CW limit = range, if no target set targetPos = -1
trueState openPercentActuator(int percent)
{
	trueState newState;
	int newMotorPos = (int)(EEPROMdeviceData.range * percent / 100);
	targetPos = newMotorPos;
	
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdeviceData.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
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
			EEPROMdeviceData.range = stepper1.currentPosition();
			newState = idleActuator(closed);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
}


//====================================================================================================================================
