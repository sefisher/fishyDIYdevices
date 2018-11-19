//------------------------------------------------------------------------------
//--------------------------------------- WiFi----------------------------------
// -----------------------------------------------------------------------------
void WiFiSetup()
{
	wifiConnect.connect = false;  
	if(DEBUG_WIFI_MESSAGES) {
		Serial.println("\n---------------------\n[WiFiSetup] Configuring wifi...");
	}
	bool result = loadCredentials();
	if(!result){
		if(DEBUG_WIFI_MESSAGES) {Serial.println("No WiFi credentials found.  Going into Soft AP mode to accept new WiFi SSID and Password.");}
		wifiConnect.connect = false;
		wifiConnect.softAPmode = true;
		runSoftAPServer(); //make softAP and serve wifi ssid pass collection page
	}else{
		if(DEBUG_WIFI_MESSAGES) {Serial.println("WiFi credentials loaded.  Going to try and connect.");}
		wifiConnect.connect=true;
		wifiConnect.softAPmode = false;
	}
}

void connectWifi() {
  if(DEBUG_WIFI_MESSAGES) {
    Serial.print("Connecting as WiFi client...Try number: ");
    Serial.println(wifiConnect.connectTryCount);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin ( wifiConnect.ssid, wifiConnect.password );
  int connRes = WiFi.waitForConnectResult();

  if(DEBUG_WIFI_MESSAGES) {Serial.print ( "\nConnection Try Result: " );Serial.println ( readStatus(connRes) );}
  
  if(connRes!=WL_CONNECTED){
    wifiConnect.connect = true;
  }else{
    wifiConnect.connect = false;
    fastBlinks(5);
    runNormalServer();
  }
}

//a wifi connection status response translator
String readStatus(int s){
    if(s==WL_NO_SHIELD) return String("WL_NO_SHIELD");
    if(s==WL_IDLE_STATUS) return String("WL_IDLE_STATUS");      
    if(s==WL_NO_SSID_AVAIL) return String("WL_NO_SSID_AVAIL");  
    if(s==WL_SCAN_COMPLETED) return String("WL_SCAN_COMPLETED");
    if(s==WL_CONNECTED) return String("WL_CONNECTED");       
    if(s==WL_CONNECT_FAILED) return String("WL_CONNECT_FAILED");    
    if(s==WL_CONNECTION_LOST) return String("WL_CONNECTION_LOST");  
    if(s==WL_DISCONNECTED) return String("WL_DISCONNECTED");  
}
// this manages trying to connect to the network
void manageConnection(){
  
    static long lastConnectTry = 0;
    int s = WiFi.status();
    
    //if not connected give it a try every 10 seconds
    if (s != WL_CONNECTED && (millis() > (lastConnectTry + 10000) ||  (wifiConnect.connectTryCount==0))) {
      wifiConnect.connect = false;
      if(DEBUG_WIFI_MESSAGES) {Serial.println ( "Connecting..." );}
      lastConnectTry = millis();
      fastBlinks(wifiConnect.connectTryCount);
      connectWifi();
      wifiConnect.connectTryCount = wifiConnect.connectTryCount + 1;
      if(wifiConnect.connectTryCount > 3){
        wifiConnect.softAPmode = true;
        runSoftAPServer();  
      }
    }
    // Detect a WLAN status change
    if (wifiConnect.status != s) { 
      if(DEBUG_WIFI_MESSAGES) {Serial.print ( "Status: " );Serial.println ( readStatus(s) );}
      wifiConnect.status = s;
      if (s == WL_CONNECTED) {
        fastBlinks(5);
        runNormalServer();
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }else if (s == 4) {
        WiFi.disconnect();
      }
    }
}

//-----------------------------------------------------------------------------
//----------------------------Websock Functions--------------------------------
//-----------------------------------------------------------------------------

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
	if (DEBUG_MESSAGES){Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);}
  	switch(type) {
		case WStype_DISCONNECTED:
			if (DEBUG_MESSAGES){Serial.printf("[%u] Disconnected!\r\n", num);}
			webSocket.sendTXT(num, "DISCONNECTED", strlen("DISCONNECTED"));
		break;
		case WStype_CONNECTED:
			{
				webSocket.sendTXT(num, "CONNECTED", strlen("CONNECTED"));
				IPAddress ip = webSocket.remoteIP(num);
				if (DEBUG_MESSAGES){Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);}
				
				webSocket.broadcastTXT(getNodeJSON().c_str(),strlen(getNodeJSON().c_str()));
			} 
			break;
		case WStype_TEXT:
			if (DEBUG_MESSAGES){Serial.printf("[%u] get Text: %s\r\n", num, payload);}
			executeCommands((char*)payload, WiFi.localIP());
			// send data to all connected clients
			webSocket.broadcastTXT(payload, length);
		break;
		case WStype_BIN:
			if (DEBUG_MESSAGES){Serial.printf("[%u] get binary length: %u\r\n", num, length);}
			hexdump(payload, length);

			// echo data back to browser
			webSocket.sendBIN(num, payload, length);
		break;
		default:
			if (DEBUG_MESSAGES){Serial.printf("Invalid WStype [%d]\r\n", type);}
		break;
  }
}
//send an update any connected websocket clients to update the screen.
//it only provides updates every 500 mSec unless forceUpdate = true.
//NOTE: forceUpdate=true will casue performance problems if called too frequently
void updateClients(String message){
	updateClients(message,false);
}
void updateClients(String message,bool forceUpdate){
	static unsigned long lastUpdate = millis();
	String text="MSG:"+message+"~*~*DAT:"+getNodeJSON();
	
	if ((millis() - lastUpdate > 500)||forceUpdate)
	{
		lastUpdate = millis();
		if(DEBUG_MESSAGES){Serial.println(text);}
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
//return the JSON data for this device from the device specific .ino file
	// put this fishyDevice data in a string.
	// Note - this is string will be parsed by scripts in webresources.h 
	// and is paralleled by UDPpollReply and if configuration setting data updated by the website then UDPparseConfigResponse is affected; 
	// if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):
	// {ip,motorPosAtCWset,motorPosAtCCWset,isMaster,motorPos,name,openIsCCW,port,group,note,swVer,devType,initStamp,range,timeOut,deviceTimedOut,swapLimSW}

String getNodeJSON()
{
	return getDeviceSpecificJSON();
}

//this creates the iframes for all the devices in the network, if /SWupdater then it loads those forms, otherwise it loads /ctrlPanels for each iframe
void handleRoot(AsyncWebServerRequest *request)
{
	if (DEBUG_MESSAGES)
	{
		Serial.println("\n[handleRoot]\n");
		Serial.println(String(WEBROOTSTR));
	}
	request->send_P(200,"text/html",WEBROOTSTR);
}

// Wifi config page 
void handleWifi(AsyncWebServerRequest *request) {
	if(DEBUG_WIFI_MESSAGES){Serial.println("[handleWifi] wifi");}
	
	AsyncWebServerResponse *response = request->beginResponse(200, "text/html","<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body>"
	"<h1>WiFi Configuration:</h1>"
	"\r\n<br/><form method='POST' action='wifisave'><h2>Connect to WiFi Network:</h2>"
	"<input type='text' placeholder='network' name='n'/>"
	"<br /><input type='password' placeholder='password' name='p'/>"
	"<br /><input type='submit' value='Submit New WiFi Credentials'/><br>(Submitting blank info will delete saved credentials)</form>"
	"<hr><form method='POST' action='justreboot'><h2>Reboot Device:</h2>Reboot and try to make the device reconnect using exisitng WiFi credentials:<br><input type='submit' value='Reboot Device'></form></body></html>");
	
	response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  	response->addHeader("Pragma", "no-cache");
  	response->addHeader("Expires", "-1");
  	request->send(response);
}

void handleWifiSave(AsyncWebServerRequest *request) {
  if(DEBUG_WIFI_MESSAGES){Serial.println("[handleWifiSave] wifi save");}
  if(request->hasParam("n", true)) {
    String n = request->arg("n");
    n.toCharArray(wifiConnect.ssid, sizeof(wifiConnect.ssid) - 1);
  }else{
    //TODO - do something if we don't find the SSID arg - ignore for now
  }
  if(request->hasParam("p", true)) {
    String p = request->arg("p");
    p.toCharArray(wifiConnect.password, sizeof(wifiConnect.password) - 1);
  }else{
    //TODO - do something if we don't find the password arg - ignore for now
  }
  if((wifiConnect.ssid=="")){resetWiFiCredentials();} //if SSID is blank, reset credentials
  
  String responseString = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body>"
    "<h1>Configuration updated.</h1>You will now be disconnected from this device and need to reconnect to your normal WiFi network."
    "<br><br>If the WiFi credentials work it will reboot and connect to your network. If they fail, it will return to Access Point mode "
    "after ~1 minute of trying. <br><br>If that occurs, use your WiFi settings to find \"" + String(wifiConnect.softAP_ssid) +
    "\"). Then return to \"" + apIP.toString() + "/wifi\" to reenter the network ID and password.<br><br>Note: The LED on the device blinks slowly when acting "
    "as an Access Point.<hr><a href=\"/wifi\">Return to the WiFi settings page.</a></body></html>";
  
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", responseString);
  
  //response->addHeader("Location", "wifi", true);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);
  
  saveCredentials();
  if(DEBUG_WIFI_MESSAGES){Serial.println("[handleWifiSave] save complete");}
  resetOnNextLoop=true;
}

void handleJustReboot(AsyncWebServerRequest *request) {
  if(DEBUG_WIFI_MESSAGES){Serial.println("[handleJustReboot] just reboot");}
  
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body>"
    "<h1>Rebooting Device.</h1><hr><a href=\"/wifi\">Return to the WiFi settings page.</a></body></html></body></html>");
  //response->addHeader("Location", "wifi", true);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);

  resetOnNextLoop=true;
}


void handleCtrl(AsyncWebServerRequest *request)
{
	if (DEBUG_MESSAGES)
	{
		Serial.println("\n[handleCtrl]\n");
		Serial.println(String(WEBCTRLSTR));
	}
	request->send_P(200,"text/html",WEBCTRLSTR);
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

void resetWiFiCredentials(){
    if(DEBUG_WIFI_MESSAGES) {Serial.println("[resetWiFiCredentials] Resetting Credentials");}
    WiFi.disconnect();
    wifiConnect.ssid[0]=0;
    wifiConnect.password[0]=0;
    saveCredentials();
    resetOnNextLoop=true;
}

//function to setup normal functioning webpages and other net protocals after connecting to wifi
void runNormalServer(){
	WiFi.setAutoReconnect(true);
	
	Udp.begin(UDP_LOCAL_PORT); //start listening on UDP port for node-node traffic
	UDPbroadcast();			   //get a poll going
	
	webSocket.begin();					//turn on websocket processing
  	webSocket.onEvent(webSocketEvent);  //set a event handler for websocket comms

	if(DEBUG_WIFI_MESSAGES) {
		Serial.print("\n--------------------------\nConnected to SSID: " );
		Serial.println (wifiConnect.ssid );
		Serial.print ( "IP address: " );
		Serial.println ( WiFi.localIP() );
	}

	String hostName;
	if (EEPROMdata.master)
	{
		hostName = "fishyDIY";
	}
	else
	{
		hostName = "fishyDIYNode" + String(WiFi.localIP()[3]);
	}
	MDNS.begin(hostName.c_str());	//start mDNS to fishyDIYmaster.local

	httpServer.on("/", handleRoot);
	httpServer.on("/SWupdater", handleRoot);
	httpServer.on("/control", handleCtrl);
	httpServer.on("/SWupdateDevForm", HTTP_GET, handleSWupdateDevForm);
	httpServer.on("/SWupdateGetForm", HTTP_GET, handleSWupdateDevForm);
	httpServer.on("/SWupdatePostForm", HTTP_POST, handleSWupdateDevPostDone,  handleSWupdateDevPost);
	httpServer.on("/network.JSON", handleNetworkJSON);
	httpServer.on("/node.JSON", handleNodeJSON);
	httpServer.on("/styles.css",handleCSS);
	httpServer.on("/wifi", handleWifi);
	httpServer.on("/wifisave", handleWifiSave);
	httpServer.on("/justreboot",handleJustReboot);	
	httpServer.onNotFound(handleNotFound);
	httpServer.begin();
	
	MDNS.addService("http", "tcp", 80);
	if(DEBUG_WIFI_MESSAGES) {Serial.println("HTTP server started\n--------------------------\n");}
}

//function to setup AP mode webpages (for WiFi setting only)
void runSoftAPServer(){
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(wifiConnect.softAP_ssid, wifiConnect.softAP_password);
    delay(500); // Without delay I've seen the IP address blank
    if(DEBUG_WIFI_MESSAGES) {
      Serial.print("\n--------------------------\nAP Name to Connect via WiFi: ");
      Serial.println(wifiConnect.softAP_ssid);
	  Serial.print("\AP Network Password: ");
      Serial.println(wifiConnect.softAP_password);
      Serial.print("\AP IP address: ");
      Serial.println(WiFi.softAPIP());
    } 
    httpServer.on("/", handleWifi);
    httpServer.on("/wifi", handleWifi);
    httpServer.onNotFound ( handleWifi );
    httpServer.on("/wifisave", handleWifiSave);
	httpServer.on("/justreboot",handleJustReboot);
    httpServer.begin(); // Web server start
    if(DEBUG_WIFI_MESSAGES) {Serial.println("HTTP server started\n--------------------------\n");}
}

void showWifiStatusinfo(){
	static long lastTimeForMessage = 0;
  	if(DEBUG_WIFI_MESSAGES){
    if(millis()>lastTimeForMessage + 2000){
      int s = WiFi.status();
      lastTimeForMessage = millis();
      Serial.print("Status");Serial.println(readStatus(s));
      Serial.print("wifiConnect.status: ");Serial.println(readStatus(wifiConnect.status));
      Serial.print("wifiConnect.connect: ");Serial.println(wifiConnect.connect?"true":"false");
      Serial.print("wifiConnect.softAPmode: ");Serial.println(wifiConnect.softAPmode?"true":"false");
    }
  }
}

