//------------------------------------------------------------------------------
//--------------------------------------- WiFi----------------------------------
// -----------------------------------------------------------------------------
void WiFiSetup()
{
	if(USE_WIFIMANAGER){
		AsyncWiFiManager WiFiManager(&httpServer,&dns);	
		WiFiManager.setAPCallback(configModeCallback);
		//reset saved settings (for testing)--------------------------------------------
		//WiFiManager.resetSettings();
		//------------------------------------------------------------------------------

		//if SSID and Password haven't been saved from before this opens an AP from the device
		//allowing you to connect to the device from a phone/computer by joining its "network"
		//from your wifi list - the name of the network will be the device name.
		//After first configuration it will auto connect unless things fail and it needs to be reset
		WiFiManager.autoConnect(EEPROMdata.namestr);
		
		if(!WiFiManager.autoConnect()) {
			if (DEBUG_MESSAGES){Serial.println("Failed to connect and hit timeout");}
			//reset and try again, or maybe put it to deep sleep
			ESP.reset();
			delay(1000);
		}	
	}else{
		WiFi.mode(WIFI_STA);
		WiFi.begin(SSID_CUSTOM, PASS_CUSTOM);
		if (WiFi.waitForConnectResult() != WL_CONNECTED) {
			if (DEBUG_MESSAGES){Serial.println("WiFi Failed");}
			
			while(1) {
				fastBlinks(2);
				delay(1000);
			}
		}
	}
	if (DEBUG_MESSAGES)
	{
		Serial.println("connected...");
	}

	// Connected!
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[WiFi] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
	}

	Udp.begin(UDP_LOCAL_PORT); //start listening on UDP port for node-node traffic
	UDPbroadcast();			   //get a poll going
	
	webSocket.begin();
  	webSocket.onEvent(webSocketEvent);
	
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

	//TODO - look at deleting this (and all references) once websocks are fully tested
	httpServer.on("/genericArgs", handleGenericArgs); //Associate the handler function to the path
	
	httpServer.on("/", handleRoot);
	httpServer.on("/SWupdater", handleRoot);
	httpServer.on("/control", handleCtrl);
	httpServer.on("/SWupdateDevForm", HTTP_GET, handleSWupdateDevForm);
	httpServer.on("/SWupdateGetForm", HTTP_GET, handleSWupdateDevForm);
	httpServer.on("/SWupdatePostForm", HTTP_POST, handleSWupdateDevPostDone,  handleSWupdateDevPost);
	//httpServer.on("/SWupdateDone", handleSWupdateDone);
	httpServer.on("/network.JSON", handleNetworkJSON);
	httpServer.on("/node.JSON", handleNodeJSON);
	httpServer.on("/styles.css",handleCSS);
	//httpServer.on("/test",handleTest);
	httpServer.onNotFound(handleNotFound);
	//httpUpdater.setup(&httpServer);
	httpServer.begin();

	MDNS.addService("http", "tcp", 80);
	fastBlinks(5);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("%s is now ready! Open http://%s.local/update in your browser\n", EEPROMdata.namestr, hostName.c_str());
	}
}

void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
//-----------------------------------------------------------------------------
//----------------------------Websock Functions--------------------------------
//-----------------------------------------------------------------------------

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
	Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[%u] Disconnected!\r\n", num);
			webSocket.sendTXT(num, "DISCONNECTED", strlen("DISCONNECTED"));
		break;
		case WStype_CONNECTED:
			{
				webSocket.sendTXT(num, "CONNECTED", strlen("CONNECTED"));
				IPAddress ip = webSocket.remoteIP(num);
				Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
				
				webSocket.broadcastTXT(getNodeJSON().c_str(),strlen(getNodeJSON().c_str()));
			} 
			break;
		case WStype_TEXT:
			Serial.printf("[%u] get Text: %s\r\n", num, payload);
			executeCommands((char*)payload, WiFi.localIP());
			// send data to all connected clients
			webSocket.broadcastTXT(payload, length);
		break;
		case WStype_BIN:
			Serial.printf("[%u] get binary length: %u\r\n", num, length);
			hexdump(payload, length);

			// echo data back to browser
			webSocket.sendBIN(num, payload, length);
		break;
		default:
			Serial.printf("Invalid WStype [%d]\r\n", type);
		break;
  }
}

void updateClients(String message){
	static unsigned long lastUpdate = millis();
	String text="MSG:"+message+"~*~*DAT:"+getNodeJSON();
	
	if (millis() - lastUpdate > 500)
	{
		lastUpdate = millis();
		Serial.println(text);
		webSocket.broadcastTXT(text.c_str(), strlen(text.c_str()));
	}
}

//-----------------------------------------------------------------------------
//-------------------------Web Server Functions--------------------------------
//-----------------------------------------------------------------------------

String _updaterError;

//Web Server - provide a JSON structure with all the deviceArray data
void handleNetworkJSON(AsyncWebServerRequest *request)
{
	
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		UDPbroadcast();
		request->send(200, "text/html", getNetworkJSON().c_str());

	}
	else
	{
		handleNotMaster(request);
	}
}

//Web Server - provide a JSON structure with all the deviceArray data
void handleNodeJSON(AsyncWebServerRequest *request)
{
	request->send(200, "text/html", getNodeJSON().c_str());
}

//consider for DELETION******
void handleNotMaster(AsyncWebServerRequest *request)
{
	String ipToShow = masterIP.toString();
	if(ipToShow == "0.0.0.0") {
		ipToShow = WiFi.localIP().toString();
	}
	String response = " <!DOCTYPE html> <html> <head> <title>fishDIY Device Network</title> <style> body {background-color: #edf3ff;font-family: \"Lucida Sans Unicode\", \"Lucida Grande\", sans-serif;}  .fishyHdr {align:center; border-radius:25px;font-size: 18px;     font-weight: bold;color: white;background: #3366cc; vertical-align: middle; } </style>  <body> <div class=\"fishyHdr\">Go to <a href=\"http://" + ipToShow + "\"> Master (" + ipToShow + ")</a></div> </body> </html> ";
	request->send(200, "text/html", response.c_str());
}

//return the JSON data for the entire network (limited data for each live node)
String getNetworkJSON()
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
put fishyDevice data in a string for all network nodes
Note - this is string will be parsed by scripts in webresources.h 
and is paralleled by UDPpollReply and if configuration setting data updated by the website then UDPparseConfigResponse is affected; 
if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):
{ip,name,typestr,groupstr,statusstr,inError,isMaster,dead}
*/
			temp += "{\"ip\":\"" + deviceArray[i].ip.toString() + "\",\"name\":\"" + String(deviceArray[i].name) + "\",\"typestr\":\"" + String(deviceArray[i].typestr) + "\",\"groupstr\":\"" + String(deviceArray[i].groupstr) + "\",\"statusstr\":\"" + String(deviceArray[i].statusstr) + "\",\"inError\":\"" + String(deviceArray[i].inError ? "true" : "false") + "\",\"isMaster\":\"" + String(deviceArray[i].isMaster ? "true" : "false") + "\",\"dead\":\"" + String(deviceArray[i].dead) + "\"}";
		}
	}
	temp += "]}";

	return temp;
}

String getNodeJSON()
{
	String temp;
	temp = "{\"fishyDevices\":[";
	/* 
	put this fishyDevice data in a string.
	Note - this is string will be parsed by scripts in webresources.h 
	and is paralleled by UDPpollReply and if configuration setting data updated by the website then UDPparseConfigResponse is affected; 
	if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):
	{ip,motorPosAtCWset,motorPosAtCCWset,isMaster,motorPos,name,openIsCCW,port,group,note,swVer,devType,initStamp,range,timeOut,deviceTimedOut,swapLimSW}
	*/
	temp += "{\"ip\":\"" + WiFi.localIP().toString() +
			"\",\"motorPosAtCWset\":\"" + String(EEPROMdata.motorPosAtCWset ? "true" : "false") +
			"\",\"motorPosAtCCWset\":\"" + String(EEPROMdata.motorPosAtCCWset ? "true" : "false") +
			"\",\"isMaster\":\"" + String(EEPROMdata.master ? "true" : "false") + "\",\"motorPos\":\"" + String(EEPROMdata.motorPos) +
			"\",\"deviceName\":\"" + String(EEPROMdata.namestr) + "\",\"openIsCCW\":\"" + String(EEPROMdata.openIsCCW ? "true" : "false") + "\",\"group\":\"" + String(EEPROMdata.groupstr) + 
			"\",\"note\":\"" + String(EEPROMdata.note) + "\",\"swVer\":\"" + String(EEPROMdata.swVer) + 
			"\",\"devType\":\"" + String(EEPROMdata.typestr) + "\",\"initStamp\":\"" + String(EEPROMdata.initstr) + 
			"\",\"range\":\"" + String(EEPROMdata.range) + "\",\"timeOut\":\"" + String(EEPROMdata.timeOut) + 
			"\",\"deviceTimedOut\":\"" + String(EEPROMdata.deviceTimedOut ? "true" : "false") + 
			"\",\"swapLimSW\":\"" + String(EEPROMdata.swapLimSW ? "true" : "false") + "\"}";

	temp += "]}";
	//if(DEBUG_MESSAGES){Serial.println("[getJSON] built: " + temp);}
	return temp;
}

void handleGenericArgs(AsyncWebServerRequest *request)
{ //Handler
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		IPAddress IPtoSend;
		String IPforCommand = "";
		char command[MAXCMDSZ] = "";
		String message = "Number of args received:";
		message += request->args(); //Get number of parameters
		message += "\n";			  //Add a new line
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleGenericArgs] :" + message);
		}
		for (int i = 0; i < request->args(); i++)
		{
			message += "Arg #" + String(i) + " -> "; //Include the current iteration value
			message += request->argName(i) + ": "; //Get the name of the parameter
			message += request->arg(i) + "\n";	 //Get the value of the parameter

			if (request->argName(i) == "IPCommand")
			{
				String response = request->arg(i);

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
					if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
					{
						Serial.println("[handleGenericArgs] MASTER node processing configuration change...");
					}
					executeCommands(command, WiFi.localIP());
				}
				else
				{
					if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
					{
						Serial.println("[handleGenericArgs] Sending slave node configuration change...");
					}
					Udp.beginPacket(IPtoSend, UDP_LOCAL_PORT);
					Udp.write(response.c_str());
					Udp.endPacket();
				}
			}
		}
		request->send(200, "text/html", message.c_str()); //Response to the HTTP request
	}
	else
	{
		handleNotMaster(request);
	}
}

//this creates the iframes for all the devices in the network, if /SWupdater then it loads those forms, otherwise it loads /ctrlPanels for each iframe
void handleRoot(AsyncWebServerRequest *request)
{
		
		if (DEBUG_MESSAGES)
		{
			Serial.println("\n[handleRoot]\n");
			Serial.println(String(WEBROOTSTRNEW));
		}
		request->send_P(200,"text/html",WEBROOTSTRNEW);
}


void handleCtrl(AsyncWebServerRequest *request){
	
		if (DEBUG_MESSAGES)
		{
			Serial.println("\n[handleCtrl]\n");
			Serial.println(String(WEBCTRLSTRNEW));
		}
		request->send_P(200,"text/html",WEBCTRLSTRNEW);
}

//show the SW update form for the specifc device (function for every device webserver)
void handleSWupdateDevForm(AsyncWebServerRequest *request)
{
  	//build the device info string
 	String WEBSTR_SWUPDATE_DEVICE_INFO = "Type: " + String(EEPROMdata.typestr) + "<br>Software Version:" + String(EEPROMdata.swVer) + "<br>Initialization String:" + String(EEPROMdata.initstr) + "<br>";

	String responseStr;
	responseStr = String(WEBSTR_SWUPDATE_PT1) + String(WEBSTR_SWUPDATE_DEVICE_INFO) +  String(WEBSTR_SWUPDATE_PT2);
	if (DEBUG_MESSAGES){Serial.println(responseStr);}
	request->send(200, "text/html", responseStr.c_str());
}

//show the SW update form for the specifc device (function for every device webserver)
void handleCSS(AsyncWebServerRequest *request)
{
 	if (DEBUG_MESSAGES){Serial.println("\n[handleCSS]\n");Serial.println(String(WEBSTYLESSTR));}
	request->send_P(200, "text/css", WEBSTYLESSTR);
}

// //TODO - delete this once done with testing
// void handleTest(AsyncWebServerRequest *request)
// {
//  	if (DEBUG_MESSAGES){Serial.println("\n[handleTest]Test HTML\n");}
// 	request->send(200, "text/html", "<!doctype html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles.css\"><title>fishDIY Device Network Test</title><meta content=\"width=device-width,initial-scale=1\"name=viewport></head><body><div>TEST</div></body></html>");
// }

// //TODO - delete if not referenced
// void setUpdaterError()
// {
// 	if (DEBUG_MESSAGES) Update.printError(Serial);
// 	StreamString str;
// 	Update.printError(str);
// 	_updaterError = str.c_str();
// }

void handleSWupdateDevPost(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      if (DEBUG_MESSAGES) Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
		if(Update.end(true)){
	        if (DEBUG_MESSAGES) Serial.printf("Update Successful: %uB\n", index+len);
      	} else {
        	Update.printError(Serial);
      	}
    }
  }

void handleSWupdateDevPostDone(AsyncWebServerRequest *request)
{
 	if (Update.hasError()) {
        request->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
    } else {
		if (DEBUG_MESSAGES) Serial.println("Got into Update Post Done. Delay and Restarting.");
		delay(100);
        ESP.restart();
    }
}

//web server response to other
void handleNotFound(AsyncWebServerRequest *request)
{
	handleNotMaster(request);
}
