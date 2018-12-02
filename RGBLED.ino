//THIS FILE HAS CUSTOM GLOBAL VARIABLES, EXTERNAL, AND INTERNAL FUNCTIONS FOR RGBLED

//=====================================================================================================================
//BEGIN CUSTOM EXTERNAL DEVICE FUNCTIONS For RGBLED - (THESE ARE CALLED BY TRANSLATOR IN "CommonDeviceTranslator.ino")  

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A RGBLED
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
String RGBgetStatusString(){
   	String statusStr = "Current Color:" + String(RGBEEPROMdeviceData.rgb);
	if(DEBUG_MESSAGES){Serial.println((String("[getStatusString]") + String(statusStr)));}
   	return statusStr;	
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
//encode compiled (settings) device data into the char[256] for storage in EEPROMdata for new devices
void RGBinitializeDeviceCustomData(){
	    String builder = String("rgb=#000000"); //make the default string (off)
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);		//copy into the main EEPROMdata storage struct
		extractDeviceCustomData();	//decode the data into the custom device struct (initialize the device)
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
//extract custom device data from char[256] in EEPROMdata and put it into the device specific struct
void RGBextractDeviceCustomData(){
	//example EEPROMdata.deviceCustomData = "rgb=#FF0000";
    char *strings[2]; //one string for each label and one string for each value
    char *ptr = NULL;
    byte index = 0;
    
	if(DEBUG_MESSAGES){Serial.println("[RGBextractDeviceCustomData]" + String(EEPROMdata.deviceCustomData));}
    
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
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
//encode dynamic device data into the char[256] for storage in EEPROMdata
void RGBencodeDeviceCustomData(){
	    String builder = "rgb=" + String(RGBEEPROMdeviceData.rgb);
		strncpy(EEPROMdata.deviceCustomData, builder.c_str(), 256);
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
//encode dynamic device data into the char[256] for storage in EEPROMdata
void RGBshowEEPROMdevicePersonalityData(){
	if(DEBUG_MESSAGES){
		Serial.println("[SETUP-device] rgb: "+String(RGBEEPROMdeviceData.rgb));  	
	}
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
//return if the device is calibrated (the common function checks for deviceTimedOut)
bool RGBisCustomDeviceReady(){
	return true; //N/A for this device
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
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
//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
void RGBoperateDevice()
{
	//if color is settled for COLOR_SAVE_WAIT mSec then encode the color into the deviceCustomData string 
	if(!saveComplete && ((lastColorUpdateTime + COLOR_SAVE_WAIT) < millis())){
		if(DEBUG_MESSAGES){Serial.println("[RGBoperateDevice] Saving color to EEPROM");}
		RGBencodeDeviceCustomData();
		storeDataToEEPROM();
		saveComplete = true;
	}
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//THIS IS A FUNCTION FOR A RGBLED
void RGBdeviceSetup()
{
	//pinSetup:
	//set switch pins to putput
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

	//test write
    digitalWrite(LED_RED, 0);
    digitalWrite(LED_GREEN, 0);
    digitalWrite(LED_BLUE, 0);
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
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
	if (cmd.startsWith("#"))
	{ 
		if((lastColorUpdateTime + COLOR_DELAY) < millis()){
			lastColorUpdateTime = millis();
			if (DEBUG_MESSAGES)
			{
				Serial.println("[RGBexecuteCommands] Commanded new color");
			}
						
			updateClients("Color Received", true);
			RGBnewColor(cmd,true); //set new color
		}
	}
	else if (cmd.startsWith("off"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded OFF");
		}
		updateClients("Off Received", true);
		executeState(false); //WiFi "false" is off 
	}
	else if (cmd.startsWith("on"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded ON");
		}
		updateClients("On Received", true);
		executeState(true); //WiFi "true" is on
	}
	else if (cmd.startsWith("hi")||cmd.startsWith("hello"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Hello! My IP is:");
			Serial.println(WiFi.localIP().toString());
			Serial.println("[RGBexecuteCommands] Here is my personality info (ignore the [Setup] tag):");
			showEEPROMPersonalityData();
		}
	}
	else if (cmd.startsWith("reset_wifi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded RESET_WIFI");
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
			Serial.println("[RGBexecuteCommands] Commanded RESET");
		}
		updateClients("Rebooting Device",true);
		resetOnNextLoop=true;
	}
	else if (cmd.startsWith("anyfishydev_there"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded ANYFISHYDEV_THERE");
		}
		UDPpollReply(remote);
	}
	else if (cmd.startsWith("poll_net"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded POLL_NET");
		}
		UDPbroadcast();
	}
	else if (cmd.startsWith("poll_response"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded POLL_RESPONSE");
		}
		UDPparsePollResponse(String(inputMsg), remote); //want the original case for this
	}
	else if (cmd.startsWith("fishydiymaster"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded FISHYDIYMASTER");
		}
		masterIP = remote; //update the master IP
	}
	else if (cmd.startsWith("config"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[RGBexecuteCommands] Commanded CONFIG");
		}
		UDPparseConfigResponse(inputMsg, remote); //want the original case for this
		updateClients("Settings Updated.", true);
	}
	else
	{
		if (DEBUG_MESSAGES){Serial.printf("[RGBexecuteCommands] Input: %s is not a recognized command.\n", inputMsg);}
	}
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (CALLED BY TRANSLATOR FUNCTION)
//This function takes updated confiuguration messages from remote controllers and updates the device
void RGBUDPparseConfigResponse(char inputMsg[MAXCMDSZ], IPAddress remote){
	char response[MAXCMDSZ] = "";
	strncpy(response,inputMsg+7,MAXCMDSZ-7); //strip off "CONFIG;"
	
	//example string = "isMaster=false;devName=RGB LED Test;groupName=;note=;timeOut=60";
    char *strings[10]; //one string for each label and one string for each value
    char *ptr = NULL;
    byte index = 0;
    
	if(DEBUG_MESSAGES){Serial.println("[RGBUDPparseConfigResponse] Got this: " + String(response));}
    
	ptr = strtok(response, "=;");  // takes a list of delimiters and points to the first token
    while(ptr != NULL)
    {
        strings[index] = ptr;
        index++;
        ptr = strtok(NULL, "=;");  // goto the next token
    }
    
    if(DEBUG_MESSAGES){for(int n = 0; n < index; n++){Serial.print(n);Serial.print(") ");Serial.println(strings[n]);}}

	//names are even (0,2,4..), data is odd(1,3,5..)
	//isMaster
	EEPROMdata.master = strcmp(strings[1],"false")==0 ? false : true;
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES){
		Serial.print("[RGBUDPparseConfigResponse] isMaster: ");
		Serial.println(EEPROMdata.master ? "true" : "false");
	}
	//devName
	strncpy(EEPROMdata.namestr, strings[3], 41);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[RGBUDPparseConfigResponse] devName: " + String(EEPROMdata.namestr));
	}
	//groupName
	strncpy(EEPROMdata.groupstr, strings[5], 41);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[RGBUDPparseConfigResponse] groupName: " + String(EEPROMdata.groupstr));
	}
	//note
	strncpy(EEPROMdata.note, strings[7], 56);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[RGBUDPparseConfigResponse] note: " + String(EEPROMdata.note));
	}
	//timeOut
	EEPROMdata.timeOut = atoi(strings[9]);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[RGBUDPparseConfigResponse] timeOut: " + String(EEPROMdata.timeOut));
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
		Serial.printf("[RGBexecuteState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (state)
	{
		RGBnewColor(RGBEEPROMdeviceData.rgb,false);
	}
	else
	{
		RGBnewColor("#000000",false);
	}
}
//=====================================================================================================================


//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL

void RGBnewColor(String cmd,bool save){
	uint32_t rgb = (uint32_t) strtol((const char *) &cmd[1], NULL, 16);

	//use bit shift operations to get the 0-255 (0-FF in HEX) range for R, G, and B.  Multiply by 4 since ESP8266's have a 10bit PWM (instead of an 8 bit PWM in Arduino)
	//so 4* scales it to 0-1020 (close enough to the 1023 max).  
	analogWrite(LED_RED,    4*((rgb >> 16) & 0xFF)); 
	analogWrite(LED_GREEN,  4*((rgb >> 8) & 0xFF));
	analogWrite(LED_BLUE,   4*((rgb >> 0) & 0xFF));
	if(DEBUG_MESSAGES){Serial.printf("RED %d, BLUE %d, GREEN %d\n",4*((rgb >> 16) & 0xFF),4*((rgb >> 8) & 0xFF),4*((rgb >> 0) & 0xFF));}

	//if just turning off or on don't save
	if(save){
		strncpy(RGBEEPROMdeviceData.rgb, cmd.c_str(), 8);
		saveComplete = false;
	}
}
//=====================================================================================================================
