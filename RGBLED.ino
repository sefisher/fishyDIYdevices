//THIS FILE HAS CUSTOM GLOBAL VARIABLES, EXTERNAL, AND INTERNAL FUNCTIONS FOR 2-STATE-ACTUATORS

//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - EXTERNAL - For RGBLED 
//COMMON EXT functions are:
// void {operateDevice(),deviceSetup(),executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)),executeState(bool state),UDPparseConfigResponse(String responseIn, IPAddress remote)}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
String RGBgetStatusString(){
   	String statusStr = "Current Color:" + String(RGBEEPROMdeviceData.rgb);
	if(DEBUG_MESSAGES){Serial.println((String("[getStatusString]") + String(statusStr)));}
   	return statusStr;	
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//encode compiled (settings) device data into the char[256] for storage in EEPROMdata for new devices
void RGBinitializeDeviceCustomData(){
	    String builder = String("rgb=#000000"); //make the default string (off)
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);		//copy into the main EEPROMdata storage struct
		extractDeviceCustomData();	//decode the data into the custom device struct (initialize the device)
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//extract custom device data from char[256] in EEPROMdata and put it into the device specific struct
void RGBextractDeviceCustomData(){
	//example EEPROMdata.deviceCustomData = "rgb=#FF0000";
    char *strings[2]; //one string for each label and one string for each value
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
	strncpy(RGBEEPROMdeviceData.rgb, strings[1], 8);		

    if(DEBUG_MESSAGES){
		String builder = "rgb=" + String(RGBEEPROMdeviceData.rgb);
		Serial.println(builder);
		showEEPROMdevicePersonalityData();
	}
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//encode dynamic device data into the char[256] for storage in EEPROMdata
void RGBencodeDeviceCustomData(){
	    String builder = "rgb=" + String(RGBEEPROMdeviceData.rgb);
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//encode dynamic device data into the char[256] for storage in EEPROMdata
void RGBshowEEPROMdevicePersonalityData(){
	if(DEBUG_MESSAGES){
		Serial.println("[SETUP-device] rgb: "+String(RGBEEPROMdeviceData.rgb));  	
	}
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//return if the device is calibrated (the common function checks for deviceTimedOut)
bool RGBisCustomDeviceReady(){
	return true; //N/A for this device
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//return a JSON with data for device
String RGBgetDeviceSpecificJSON(){
	String temp;
	temp = "{\"fishyDevices\":[";
	/* 
	put this RGBLED data in a string.
	Note - this is string will be parsed by scripts in the custom device webpage (webresource.h) - if adding data elements all these may need updating.  
	This function sends data as follows (keep this list updated):
	{ip,isMaster,name,group,note,swVer,devType,initStamp,timeOut,deviceTimedOut}
	*/
	temp += "{\"ip\":\"" + WiFi.localIP().toString() +
			"\",\"isMaster\":\"" + String(EEPROMdata.master ? "true" : "false") +
			"\",\"deviceName\":\"" + String(EEPROMdata.namestr) + "\",\"group\":\"" + String(EEPROMdata.groupstr) + 
			"\",\"note\":\"" + String(EEPROMdata.note) + "\",\"swVer\":\"" + String(EEPROMdata.swVer) + 
			"\",\"devType\":\"" + String(EEPROMdata.typestr) + "\",\"initStamp\":\"" + String(EEPROMdata.initstr) + 
			"\",\"timeOut\":\"" + String(EEPROMdata.timeOut) + 
			"\",\"deviceTimedOut\":\"" + String(EEPROMdata.deviceTimedOut ? "true" : "false") + 
			"\",\"rgb\":\"" + String(RGBEEPROMdeviceData.rgb) + "\"}";
	temp += "]}";
	return temp;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void RGBoperateDevice()
{
	//TODO - make this if needed operateRGBLED();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void RGBdeviceSetup()
{
	//pinSetup:
	//set switch pins to putput
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

	//test write
    digitalWrite(LED_RED, 1);
    digitalWrite(LED_GREEN, 1);
    digitalWrite(LED_BLUE, 1);
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//This function takes messages from some remote address (if from another node)
//that are of maximum lenght MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//REQD COMMANDS: {anyfishydev_there,fishydiymaster,poll_net,poll_response,reset_wifi,reset} -> needed to be a fishyDevice on the network
//VALID DEVICE COMMANDS: 
// (open,close,stop,gotoXXX,{hi.hello},calibrate,config[;semi-colon-separated-parameters=values])
void RGBexecuteCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
	String cmd = String(inputMsg);
	cmd.toLowerCase();
	if (cmd.startsWith("off"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded OFF");
		}
		updateClients("Off Received", true);
		executeState(false); //WiFi "false" is off 
	}
	else if (cmd.startsWith("on"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded ON");
		}
		updateClients("On Received", true);
		executeState(true); //WiFi "true" is on
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
		
		//TODO - FIX THIS WIFI RESET
		resetWiFiCredentials();
		//AsyncWiFiManager WiFiManager(&httpServer,&dns);
		//WiFiManager.resetSettings();
		//resetOnNextLoop = true;
		//delay(2000);
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
		if (DEBUG_MESSAGES){Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);}
	}
}

//I'm here - TODO UPDATE THE PARSE FUNCTION AND KEEP UPDATING THIS FILE to MATCH RGBLED
// (look for any 2-state-actuator functions and update them)

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//configuration response from web/udp - this performs a configuration update to the device and stores it in EEPROM
void RGBUDPparseConfigResponse(String responseIn, IPAddress remote){
	String response = responseIn.substring(7); //strip off "CONFIG"
	int strStrt, strStop;

	//parseString in this order: {isMaster, devName, groupName, devType, note, timeOut}
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] Got this: " + responseIn);
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
void RGBexecuteState(bool state)
{
	//reset the motor timeout counter
	deviceResponseTime = millis();
	EEPROMdata.deviceTimedOut = false;
	
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (state)
	{
		
	}
	else
	{
		
	}
}
//=====================================================================================================================


//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL


//====================================================================================================================================
