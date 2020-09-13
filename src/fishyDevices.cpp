/*
fishyDevices

Copyright (C) 2020 by Stephen Fisher 

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

For Fauxmo Library - Copyright (c) 2018 by Xose Pérez <xose dot perez at gmail dot com> (also under MIT License) - see below for full licence.
For ESPAsync Toolset - Copyright (c) 2016 Hristo Gochkov (under GNU License version 2.1). All rights reserved.
*/

//TODO - for homeMonitoring : Fix multiple name convention (simpleswitch) to work
//TODO - for homeMonitoring : activity not being announced and logged when physical switch pressed - send activity if network is working

//TODO Update library.properties to add the dependencies so CLI can automatically pull them
//TODO - in the notes explaining things - make sure I explain updateClients(msg,1) vs updateClients(msg,0) - 1 sends activity to the logger and should be for final states or received commands (also forces and update even if just done recently) ,0 is for information or transient states and will not be logged as activity by the logger.
//TODO - ESP32 integration
//TODO - work on a script to pull all the variants into the examples folder
//TODO - make a global "turn off fauxmo" switch (MASTER) that sends a UDP message to disable all fauxmo (disable voice controls)

#include "fishyDevices.h"

//Constructor
fishyDevice::fishyDevice(const char *WEBCTRLSTR)
{
  deviceResponseTime = 0;
  apIP = IPAddress(192, 168, 4, 1);
  netMsk = IPAddress(255, 255, 255, 0);
  masterIP = IPAddress(0, 0, 0, 0);
  resetOnNextLoop = false;
  myWifiConnect.softAP_ssid = CUSTOM_DEVICE_NAME;
  myWifiConnect.softAP_password = SOFT_AP_PWD;
  webctrlstrPtr = WEBCTRLSTR;
  httpServer = new AsyncWebServer(80);
  dnsServer = NULL;
  webSocket = NULL;
}

//============================================
//setup, initialization, and reset functions
//============================================
void fishyDevice::FD_setup()
{
  serialStart();               //if debugging turn on serial, supports all "show" functions
  showPersonalityDataSize();   //if debugging - show the EEPROM size for all the data
  retrieveDataFromEEPROM();    //this also initializes if a new device or new initialization string is defined
  showEEPROMPersonalityData(); //show device information from memory or initialization
  WifiFauxmoAndDeviceSetup();  //setup wifi, if crenditials found-> run normal server, if not -> run AP to allow entering wifi
  deviceSetup();               //custom device setup call; *Note: runs even if in AP mode. Need to make sure it won't break things.
}

//determine if initalization string is different than stored in EEPROM -
//if so load in new data from compiled code; if not just load SW version info
//but keep stored perosnality data (software update without personality change)
void fishyDevice::initializePersonalityIfNew()
{
  if (DEBUG_MESSAGES)
  {
    Serial.printf("[initializePersonalityIfNew] New: %s Old: %s\n", String(INITIALIZE).c_str(), String(myEEPROMdata.initstr).c_str());
  }
  // change myEEPROMdata in RAM
  if (String(INITIALIZE) != String(myEEPROMdata.initstr))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("[initializePersonalityIfNew] Updating...");
    }

    //store specified personality data
    strncpy(myEEPROMdata.swVer, SW_VER, 11);
    strncpy(myEEPROMdata.initstr, INITIALIZE, 13);
    strncpy(myEEPROMdata.namestr, CUSTOM_DEVICE_NAME, MAXNAMELEN);
    strncpy(myEEPROMdata.typestr, CUSTOM_DEVICE_TYPE, MAXTYPELEN);
    strncpy(myEEPROMdata.note, CUSTOM_NOTE, MAXNOTELEN);
    myEEPROMdata.timeOut = DEVICE_TIMEOUT;
    myEEPROMdata.deviceTimedOut = false; //initialized to false on boot

    //Get the default device specific data - load it into myEEPROMdata, and decode it into the device specific EEPROMdeviceData
    //this call also stores the encoded char[MAXCUSTOMDATALEN] into myEEPROMdata.deviceCustomData
    initializeDeviceCustomData();

    if (MASTER_NODE)
    {
      if (DEBUG_MESSAGES)
      {
        Serial.println("[initializePersonalityIfNew] Setting this node as MASTER.");
      }
      myEEPROMdata.master = true;
    }
    else
    {
      myEEPROMdata.master = false;
    }
  }
  else
  {
    //always show the latest SW_VER
    strncpy(myEEPROMdata.swVer, SW_VER, 11);
    if (DEBUG_MESSAGES)
    {
      Serial.println("[initializePersonalityIfNew] Actual swVER: " + String(myEEPROMdata.swVer));
    }
    if (DEBUG_MESSAGES)
    {
      Serial.println("[initializePersonalityIfNew] Nothing else to update.");
    }
  }
}
//dump EEPROM personality data to Serial
void fishyDevice::showEEPROMPersonalityData()
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[showEEPROMPersonalityData]start");
    Serial.println("[showEEPROMPersonalityData] Init string: " + String(myEEPROMdata.initstr) + ", Name string: " + String(myEEPROMdata.namestr) + ", Master: " + String(myEEPROMdata.master ? "True" : "False") + ",Type string: " + String(myEEPROMdata.typestr) + ",Note string: " + String(myEEPROMdata.note) + ", SW Version string: " + String(myEEPROMdata.swVer) + ", Device Timeout " + String(myEEPROMdata.timeOut) + ", Device Timedout " + String(myEEPROMdata.deviceTimedOut));
    Serial.println("[showEEPROMPersonalityData] Compiled init string: " + String(INITIALIZE) + ". Stored init string: " + String(myEEPROMdata.initstr));

    showEEPROMdevicePersonalityData();

    Serial.println("[showEEPROMPersonalityData]finish");
  }
}

//dump fishyDevice data to Serial
void fishyDevice::showThisNode(fishyDeviceData holder)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[showThisNode] IP: " + holder.ip.toString() + ", Name string: " + String(holder.name) + ", Master: " + String(holder.isMaster ? "True" : "False") + ",Type string: " + String(holder.typestr) + ",Status string: " + String(holder.statusstr) + ",shortStat string: " + String(holder.shortStat) + ",X: " + String(holder.locationX) + ",Y: " + String(holder.locationY) + ",Z: " + String(holder.locationZ) + ", Error state: " + String(holder.inError ? "True" : "False") + ", Dead state: " + String(holder.dead ? "True" : "False") + ", Time Stamp: " + String(holder.timeStamp));
  }
}

//sets up a fauxmo device if enabled to allow control via Alexa, etc
void fishyDevice::WifiFauxmoAndDeviceSetup()
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[WifiFauxmoAndDeviceSetup]start");
  }
  // You can enable or disable the library at any moment
  // Disabling it will prevent the devices from being discovered and switched
  fauxmo.enable(false);
  if (DEBUG_MESSAGES)
  {
    Serial.println("[WifiFauxmoAndDeviceSetup]disabled fauxmo");
  }
  // WiFi - see wifi-and-webserver.ino
  WiFiSetup();
  if (DEBUG_MESSAGES)
  {
    Serial.println("[WifiFauxmoAndDeviceSetup]did wifi setup");
  }
  if (FAUXMO_ENABLED)
  {

    fauxmo.createServer(false); //using existing webServer
    fauxmo.setPort(80);         //required for gen3+ devices

    fauxmo.enable(true);
    // Add virtual device
    fauxmo.addDevice(myEEPROMdata.namestr);
    if (DEBUG_MESSAGES)
    {
      Serial.println("[WifiFauxmoAndDeviceSetup]enabled fauxmo");
    }
  }

  if (FAUXMO_ENABLED)
  {
    fauxmo.onSetState(std::bind(&fishyDevice::onSetStateForFauxmo, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  }
  if (DEBUG_MESSAGES)
  {
    Serial.println("[WifiFauxmoAndDeviceSetup]finish");
  }

  if(myEEPROMdata.blink_Enable)
  {
    pinMode(blinker_led, OUTPUT);
    digitalWrite(blinker_led, HIGH);
  }
}

//function to add additonal devices, device ID will be incrmented for each call (the default/first device_id is 0)
unsigned char fishyDevice::addAnotherDevice(const char *device_name){
   return fauxmo.addDevice(device_name);
}

//passthrough function for binding to fauxmo,onSetState
void fishyDevice::onSetStateForFauxmo(unsigned char device_id, const char *device_name, bool state, unsigned char value)
{
  // Callback when a command from Alexa is received.
  // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
  // State is a boolean (ON/OFF) and value a number from 001 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
  // Just remember not to delay too much here, this is a callback, exit as soon as possible.
  // If you have to do something more involved here set a flag and process it in your main loop.
  if (DEBUG_MESSAGES)
  {
    Serial.printf("[fD onSetStateForFauxmo] Device #%d (%s) state was set: %s set value was: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
  }
  
  executeState(device_id, device_name, state, value, 2); //2= context 2 which tells the device it was a fauxmo state change (has no custom device knowledge)

}

//as name implies - dump memory remaining on serial - process every second
void fishyDevice::showHeapAndProcessSerialInput()
{
  if (DEBUG_MESSAGES)
  {
    if (DEBUG_HEAP_MESSAGE)
    {
      static unsigned long last = millis();
      if (millis() - last > 1000)
      {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
      }
    }

    if (!myWifiConnect.softAPmode)
    {
      //take serial commands in
      if (use_serial && (Serial.available() > 0))
      {
        char inputMsg[MAXCMDSZ*2];
        int sizeRead;
        sizeRead = Serial.readBytesUntil('\r', inputMsg, sizeof(inputMsg));
        if (sizeRead)
        {
          executeCommands(inputMsg, IPAddress(0, 0, 0, 0),-1);
        }
        Serial.flush();
      }
    }
  }
}

// Initialize serial port, LED (visual communcations), and clean garbage
void fishyDevice::serialStart()
{
  //set default settings in case the extern const aren't set outside library

  if(USE_SERIAL_INPUT == NULL){
    use_serial = false;
  }else{
    use_serial = USE_SERIAL_INPUT;
  }

  if(BLINK_LED == NULL){
    blinker_led = LED_BUILTIN;
  }else{
    blinker_led = BLINK_LED;
  }

  if(DO_BLINKING == NULL){
    myEEPROMdata.blink_Enable = true;
  }else{
    myEEPROMdata.blink_Enable = DO_BLINKING;
  }

  if (DEBUG_MESSAGES)
  {
    if(use_serial) {
      Serial.begin(SERIAL_BAUDRATE);
    }else{
      Serial.begin(SERIAL_BAUDRATE,SERIAL_8N1, SERIAL_TX_ONLY);
    }
    Serial.println();
    Serial.println();
  }
}
//reset if commanded to by someone last loop
void fishyDevice::checkResetOnLoop()
{
  //TODO - consider adding webSocket->cleanupClients(); call every minute or so (see https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients)

  if (resetOnNextLoop)
  {
    //allow time for any commit to settle and webresponses to process before bailing
    if (DEBUG_MESSAGES)
    {
      Serial.println("[checkResetOnLoop] Reset Is Required...delay, then reset.");
    }
    delay(1000);
    resetController();
  }
  if(myEEPROMdata.blink_Enable && (fastBlinkCount || slowBlinkCount)){
    doBlinking();
  }
}
//Setup Device Personaility and update EEPROM data as needed
void fishyDevice::showPersonalityDataSize()
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[SETUP] The personality data needs " + String(sizeof(myEEPROMdata)) + " Bytes in EEPROM.");
    Serial.println("[SETUP] initstr " + String(sizeof(myEEPROMdata.initstr)));
    Serial.println("[SETUP] namestr " + String(sizeof(myEEPROMdata.namestr)));
    Serial.println("[SETUP] master " + String(sizeof(myEEPROMdata.master)));
    Serial.println("[SETUP] typestr " + String(sizeof(myEEPROMdata.typestr)));
    Serial.println("[SETUP] note " + String(sizeof(myEEPROMdata.note)));
    Serial.println("[SETUP] swVer " + String(sizeof(myEEPROMdata.swVer)));
    Serial.println("[SETUP] timeOut " + String(sizeof(myEEPROMdata.timeOut)));
    Serial.println("[SETUP] deviceTimedOut " + String(sizeof(myEEPROMdata.deviceTimedOut)));
    Serial.println("[SETUP] deviceCustomData " + String(sizeof(myEEPROMdata.deviceCustomData)));
    Serial.println("[SETUP] blink_Enable " + String(sizeof(myEEPROMdata.blink_Enable)));
    Serial.println("[SETUP] reserved_data_package " + String(sizeof(myEEPROMdata.reserved_data_package)));
    Serial.println("[SETUP] locationX " + String(sizeof(myEEPROMdata.locationX)));
    Serial.println("[SETUP] locationY " + String(sizeof(myEEPROMdata.locationY)));
    Serial.println("[SETUP] locationZ " + String(sizeof(myEEPROMdata.locationZ)));
  }
}

void fishyDevice::resetController()
{
  WiFi.disconnect(true);
  delay(2000);
  if (DEBUG_MESSAGES)
  {
    Serial.println("[resetController] Restarting.");
  }
  ESP.restart();
}

void fishyDevice::fastBlinks(int numBlinks)
{
  slowBlinkCount = 0;
  fastBlinkCount = numBlinks;
  fastBlinkState = HIGH;
  digitalWrite(blinker_led, slowBlinkState);
  fastBlinkStart = millis();
}


void fishyDevice::slowBlinks(int numBlinks)
{
  fastBlinkCount = 0;
  slowBlinkCount = numBlinks;
  slowBlinkState = HIGH;
  digitalWrite(blinker_led, slowBlinkState);
  slowBlinkStart = millis();
}

void fishyDevice::doBlinking()
{
  unsigned long currentmillis = millis();
  if(fastBlinkCount){
    if(currentmillis - fastBlinkStart >= 100){
      fastBlinkStart = currentmillis;
      if(fastBlinkState == HIGH){
        fastBlinkState = LOW;
      }else{
        if(DEBUG_MESSAGES){
          Serial.print(currentmillis);
          Serial.print(" fastBlinkCount = ");
          Serial.println(fastBlinkCount);
        }
        fastBlinkCount--;  
        fastBlinkState = HIGH;
      }
      digitalWrite(blinker_led, fastBlinkState);
    }
  }
  else if(slowBlinkCount){
    if(currentmillis - slowBlinkStart >= 1000){
      slowBlinkStart = currentmillis;
      if(slowBlinkState == HIGH){
        slowBlinkState = LOW;
      }else{
        if(DEBUG_MESSAGES){
          Serial.print(currentmillis);
          Serial.print(" slowBlinkCount = ");
          Serial.println(slowBlinkCount);
        }
        slowBlinkCount--;  
        slowBlinkState = HIGH;
      }
      digitalWrite(blinker_led, slowBlinkState);
    }
  }
    
}

//-------------------------------------------------------
//TODO - CLEANUP look to see if these functions should be deleted

//makes the name string 255 in length and adds H3 tags (space after tags so ignored by browser)
String fishyDevice::paddedH3Name(String name)
{
  String newName = "<H3>" + name + "</H3>";
  int growBy = 255 - name.length();
  for (int i = 0; i < growBy; i++)
  {
    newName += " ";
  }
  return newName;
}

String fishyDevice::threeDigits(int i)
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

String fishyDevice::paddedInt(int lengthInt, int val)
{
  String paddedStr = String(val);
  while (paddedStr.length() < lengthInt)
  {
    paddedStr += " ";
  }
  return paddedStr;
}

String fishyDevice::paddedIntQuotes(int lengthInt, int val)
{
  String paddedStr = "'" + String(val) + "'";
  while (paddedStr.length() < lengthInt + 2)
  {
    paddedStr += " ";
  }
  return paddedStr;
}

// - end consider
//-----------------------------------------------------

//double the command size to allow longer configuration strings
void fishyDevice::executeCommands(char inputMsg[MAXCMDSZ*2], IPAddress remote, int client_id)
{
  String cmd = String(inputMsg);
  String test;
  cmd.toLowerCase();
  if (executeDeviceCommands(inputMsg, remote))
  {
    Serial.println("[executeCommands]Ran Device Command. Done.");
  }
  else if (cmd.startsWith("hi") || cmd.startsWith("hello"))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("[executeCommands] Hello! My IP is:");
      Serial.println(WiFi.localIP().toString());
      Serial.println("[executeCommands] Here is my personality info:");
      showEEPROMPersonalityData();
    }
  }
  else if (cmd.substring(0,strlen(UDP_NEWLOCATION)).equalsIgnoreCase(String(UDP_NEWLOCATION)))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println(String("[executeCommands] Commanded " + String(UDP_NEWLOCATION)));
    }
    updateClients("Updating Device Location", true);
    updateLocation(inputMsg);
  }
  else if (cmd.startsWith("get_network_json"))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("[executeCommands] Commanded GET_NETWORK_JSON");
      Serial.println(getNetworkJSON().c_str());
    }
    //This function is generally called by the homeMonitor site asking for an updated list of all the devices statusses.
    updateSpecificClient(getNetworkJSON().c_str(), client_id);
    updateSpecificClient("COMPLETE", client_id);
  }
  else if (cmd.startsWith("reset_wifi"))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("[executeCommands] Commanded RESET_WIFI");
    }
    updateClients("Resetting WiFi", true);
    resetWiFiCredentials();
  }
  else if (cmd.startsWith("reset"))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("[executeCommands] Commanded RESET");
    }
    updateClients("Rebooting Device", true);
    resetOnNextLoop = true;
  }
  else if (cmd.substring(0,strlen(UDP_YO)).equalsIgnoreCase(String(UDP_YO)))
  {
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.println(String("[executeCommands] Commanded ") + String(UDP_YO));
    }
    UDPpollReply(remote);
  }
  else if (cmd.substring(0,strlen(UDP_HERE)).equalsIgnoreCase(String(UDP_HERE)))
  {
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.println(String("[executeCommands] Commanded") + String(UDP_HERE));
    }
    UDPparsePollResponse(inputMsg, remote); //want the original case for this
  }
  else if (cmd.substring(0,strlen(UDP_MASTER)).equalsIgnoreCase(String(UDP_MASTER)))
  {
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.print(String("[executeCommands] Commanded " + String(UDP_MASTER) + String(" from: ")));
      Serial.println(remote);
    }
    masterIP = remote; //update the master IP
  }
  else if (cmd.substring(0,strlen(UDP_LOGGER)).equalsIgnoreCase(String(UDP_LOGGER)))
  {
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.print(String("[executeCommands] Commanded " + String(UDP_LOGGER)));
      Serial.println(remote);
    }
    loggerIP = remote; //update the master IP
    UDPackLogger();
  }
  else if (cmd.substring(0,strlen(UDP_CONFIG_REQUEST)).equalsIgnoreCase(String(UDP_CONFIG_REQUEST)))
  {
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.print(String("[executeCommands] Commanded " + String(UDP_CONFIG_REQUEST)));
      Serial.println(remote);
    }
    UDPconfigRequest();
  }
  else if (cmd.substring(0,strlen(UDP_ACTIVITY)).equalsIgnoreCase(String(UDP_ACTIVITY)))
  {
    UDPhandleActivityMessage(inputMsg, remote); //want the original case for this
  }
  else
  {
    if (DEBUG_MESSAGES)
    {
      Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);
    }
  }
}
//send in string with
void fishyDevice::updateLocation(char inputMsg[MAXCMDSZ])
{
  char response[MAXCMDSZ] = "";
  int len = strlen(UDP_NEWLOCATION) + 1;
  strncpy(response, inputMsg + len, MAXCMDSZ - len); //strip off UDP_NEWLOCATION

  //parseString in this order: {x,y,z}}
  //example string = "LOCATION_CHANGE:X=40;Y=50;Z=1" ->"X=40;Y=50;Z=1"
  char *strings[6]; //one string for each label and one string for each value
  char *ptr = NULL;
  byte index = 0;

  if (DEBUG_MESSAGES)
  {
    Serial.println("[updateLocation] Got this: " + String(response));
  }

  ptr = strtok(response, "=;"); // takes a list of delimiters and points to the first token
  while (ptr != NULL)
  {
    strings[index] = ptr;
    index++;
    ptr = strtok(NULL, "=;"); // goto the next token
  }

  if (DEBUG_MESSAGES)
  {
    for (int n = 0; n < index; n++)
    {
      Serial.print(n);
      Serial.print(") ");
      Serial.println(strings[n]);
    }
  }

  //names are even (0,2,4..), data is odd(1,3,5..)
  //X
  myEEPROMdata.locationX = atoi(strings[1]);

  //Y
  myEEPROMdata.locationY = atoi(strings[3]);

  //Z
  myEEPROMdata.locationZ = atoi(strings[5]);

  if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
  {
    Serial.println("[updateLocation] X Y Z: " + String(myEEPROMdata.locationX) + " " + String(myEEPROMdata.locationY) + " " + String(myEEPROMdata.locationZ));
  }

  storeDataToEEPROM();
}
//==================================
// UDP Communication functions
//==================================
//process UDP packets
void fishyDevice::UDPprocessPacket()
{
  //USED FOR UDP COMMS
  char packetBuffer[MAXCMDSZ*2 + 56 + MAXNAMELEN]; //buffer to hold incoming packet (MAXCMDSZ with potential to have MASTER_COMMAND_DATA and an IP added.)

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    IPAddress remoteIp = Udp.remoteIP();
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.print("[UDPprocessPacket] Received packet of size ");
      Serial.println(packetSize);
      Serial.print("[UDPprocessPacket] From ");
      Serial.print(remoteIp);
      Serial.print(", port ");
      Serial.println(Udp.remotePort());
    }
    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, MAXCMDSZ*2 + 56 + MAXNAMELEN);
    if (len > 0)
    {
      packetBuffer[len] = 0;
    }
    if (DEBUG_UDP_MESSAGES)
    {
      Serial.print("[UDPprocessPacket] Processing:");
      Serial.println(packetBuffer);
    }
    executeCommands(packetBuffer, remoteIp,-1);
  }
}

//if master announce every minute and cull nodes if silent for 10 min;
//if not master annouce every 8 to avoid being culled
void fishyDevice::UDPkeepAliveAndCull()
{
  //announce the master every 2 minutes
  if (myEEPROMdata.master)
  {
    static unsigned long lastMstr = millis();
    if (millis() - lastMstr > 120000)
    {
      lastMstr = millis();
      if (DEBUG_MESSAGES)
      {
        Serial.println("[MAIN] Announcing master");
      }
      UDPannounceMaster();
    }
    static unsigned long lastNodeCulling = millis();
    if (millis() - lastNodeCulling > 600000) //cull dead nodes from the list every 10 minutes
    {
      lastNodeCulling = millis();
      if (DEBUG_MESSAGES)
      {
        Serial.println("[MAIN] Removing Dead Nodes");
      }
      cullDeadNodes();
    }
    static unsigned long lastAvoidCulling = millis();
    if (millis() - lastAvoidCulling > 240000) //poll the net every 4 minutes to avoid culling nodes
    {
      lastAvoidCulling = millis();
      if (DEBUG_MESSAGES)
      {
        Serial.println("[MAIN] Broadcasting for Poll");
      }
      UDPbroadcast();
    }
  }
}

//at end of setup annouce your presence
void fishyDevice::announceReadyOnUDP()
{
  //announce master if this is the mastr node
  //otherwise let the master know you're here
  if (myEEPROMdata.master)
  {
    if(DEBUG_MESSAGES){Serial.println("Connected. Announcing Master on UDP.");}
    UDPannounceMaster();
  }
  else
  {
    if(DEBUG_MESSAGES){Serial.println("Connected. Broadcasting to see who is on network via UDP.");}
    UDPbroadcast();
  }
}

//acknowledge loggerIR
void fishyDevice::UDPackLogger()
{
  IPAddress broadcast = WiFi.localIP();
  broadcast[3] = 255;
  Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
  Udp.write(String(String(UDP_ACK_LOGGER) + String(": ") + loggerIP.toString()).c_str());
  Udp.endPacket();
}

//provide a config string if requested via UDP
void fishyDevice::UDPconfigRequest(){
  IPAddress broadcast = WiFi.localIP();
  broadcast[3] = 255;
  Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
  Udp.write(String(String(UDP_CONFIG_RESPONSE) + String(": ") + getConfigString()).c_str());
  Udp.endPacket();
}

//broadcast on subnet to see who will respond
void fishyDevice::UDPbroadcast()
{
  IPAddress broadcast = WiFi.localIP();
  broadcast[3] = 255;

  //process this devices data first, storing it in the deviceArray, then poll the rest of the network
  dealWithThisNode(makeMyfishyDeviceData());

  Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
  Udp.write(UDP_YO);

  Udp.endPacket();
}

void fishyDevice::UDPannounceMaster()
{
  IPAddress broadcast = WiFi.localIP();
  broadcast[3] = 255;
  Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
  Udp.write(UDP_MASTER);
  Udp.endPacket();
}

//put out this devices data on the net
//this tells the master to update the 
//list of devices pulled by network.JSON
void fishyDevice::UDPpollReply(IPAddress remote)
{
  fishyDeviceData holder; //temp
  holder = makeMyfishyDeviceData();
  String response = String(UDP_HERE) + ":";
  Udp.beginPacket(remote, UDP_LOCAL_PORT);
/* 
send fishyDeviceData data.
Note - this is parsed by UDPparsePollResponse and paralleled by getJSON; if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):{ip,name,typestr,statusstr,inError,isMaster,shortStat,locationX,locationY,locationZ}
*/
  response += "{" + holder.ip.toString() + "," +
              String(holder.name) + "," +
              String(holder.typestr) + "," +
              String(holder.statusstr) + "," +
              String(holder.inError ? "true" : "false") + "," +
              String(holder.isMaster ? "true" : "false") + "," +
              String(holder.shortStat) + "," +
              String(holder.locationX) + "," +
              String(holder.locationY) + "," +
              String(holder.locationZ) + "}";

  Udp.write(response.c_str());
  Udp.endPacket();

  if (myEEPROMdata.master)
  {
    UDPannounceMaster();
  } //make sure they know who is in charge
}

//parses a UDP poll reply and takes action
void fishyDevice::UDPparsePollResponse(char inputMsg[MAXCMDSZ*2], IPAddress remote)
{
  if (myEEPROMdata.master)
  {
/* 
parse fishyDeviceData data.
Note - this data set is sent by UDPparsePollResponse and getNetworkJSON; UDPparseConfigResponse may be affected as well (if data is added that needs set by configuration updates)
it is also parsed by scripts in wifi-and-webserver.ino and webresources.h if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):{ip,name,typestr,statusstr,inError,isMaster,shortStat,locationX,locationY,locationZ}
*/

    //example input: "[UDP_HERE]{10.203.1.133,Rotator,Limit-SW-Actuator,Current Position:25% Open,false,false,25 ,134,216,1}

    char response[MAXCMDSZ*2] = "";
    int len = strlen(UDP_HERE) + 1;
    strncpy(response, inputMsg + len, MAXCMDSZ*2 - len); //strip off UDP_HERE
    if (DEBUG_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] Got this: " + String(response));
    }

    char *strings[10]; //one string for each value
    char *ptr = NULL;
    byte index = 0;

    fishyDeviceData holder;
    holder.dead = false;
    ptr = strtok(response, "{,}"); // takes a list of delimiters and points to the first token

    while (ptr != NULL)
    {
      strings[index] = ptr;
      index++;
      ptr = strtok(NULL, "{,}"); // goto the next token
    }

    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] found this many parts: " + String(index));
    }

    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      for (int n = 0; n < index; n++)
      {
        Serial.print(n);
        Serial.print(") ");
        Serial.println(strings[n]);
      }
    }

    //IP
    String strIP = String(strings[0]);
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

    //name
    holder.name = String(strings[1]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] strName: " + holder.name);
    }

    //typestr
    holder.typestr = String(strings[2]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] typestr: " + holder.typestr);
    }

    //statusstr
    holder.statusstr = String(strings[3]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] statusstr: " + holder.statusstr);
    }

    //inError
    holder.inError = strcmp(strings[4], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.print("[UDPparsePollResponse] inError: ");
      Serial.println(holder.inError ? "true" : "false");
    }

    //isMaster
    holder.isMaster = strcmp(strings[5], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.print("[UDPparsePollResponse] isMaster: ");
      Serial.println(holder.isMaster ? "true" : "false");
    }

    //shortStat
    holder.shortStat = String(strings[6]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] shortStat: " + holder.shortStat);
    }

    //locationX
    holder.locationX = atoi(strings[7]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] X: " + holder.locationX);
    }
    //locationY
    holder.locationY = atoi(strings[8]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] Y: " + holder.locationY);
    }
    //locationZ
    holder.locationZ = atoi(strings[9]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
      Serial.println("[UDPparsePollResponse] Y: " + holder.locationZ);
    }

    dealWithThisNode(holder); //update the list of nodes
  }
}
void fishyDevice::UDPhandleActivityMessage(char inputMsg[MAXCMDSZ*2], IPAddress remote)
{
  if (myEEPROMdata.master)
  {

    //pass on message to broadcast for logger
    //example input: "[UDP_ACTIVITY]:device=10.203.1.33;message=Commanded to Open;"
    char response[MAXCMDSZ*2] = UDP_MSG4LOG;    
    strncpy(response + strlen(UDP_MSG4LOG), inputMsg + strlen(UDP_ACTIVITY), MAXCMDSZ*2 - strlen(UDP_MSG4LOG)); //prepend UDP_MSG4LOG message and strip off UDP_MSG4LOG
    
    if (DEBUG_MESSAGES)
    {
      Serial.println("[UDPhandleActivityMessage] Got this: " + String(response));
    }
    if (loggerIP.toString() != "0.0.0.0" && loggerIP.toString() != "(IP unset)")
    {
      IPAddress broadcast = WiFi.localIP();
      broadcast[3] = 255;
      Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
      Udp.write(response);
      Udp.endPacket();
    }
  }
}
//==================================================================
// functions to maintain list of active fishyDevices on network
//==================================================================
//-------------------------------------------------------------------------
//-------deviceArray funtions---------------------------
//-------------------------------------------------------------------------

//return a fishyDeviceData with this devices status in it.
//used primarily to add device data to array of device data Stored
//for webserver
fishyDeviceData fishyDevice::makeMyfishyDeviceData()
{
  bool isReady = false; //used for the custom device functions to report any errors to include being uncalibrated

  fishyDeviceData holder;

  //fill with current data
  holder.dead = false;
  holder.timeStamp = millis();
  holder.ip = WiFi.localIP();

  isReady = isCustomDeviceReady();

  holder.isMaster = myEEPROMdata.master;

  if (myEEPROMdata.deviceTimedOut || !(isReady))
  {
    holder.inError = true;
  }
  else
  {
    holder.inError = false;
  }

  holder.name = String(myEEPROMdata.namestr);
  holder.typestr = String(myEEPROMdata.typestr);
  holder.statusstr = getStatusString(); //ask the device for a status string
  holder.shortStat = getShortStatString(); //ask the device for a short status string
  holder.locationX = myEEPROMdata.locationX;
  holder.locationY = myEEPROMdata.locationY;
  holder.locationZ = myEEPROMdata.locationZ;

  //if debuging show the data
  showThisNode(holder);

  return holder;
}

//take an address for a node and figure out what to do with it
//return the index on success or -1 on fail
int fishyDevice::dealWithThisNode(fishyDeviceData netDevice)
{
  //don't add a 0.0.0.0 device to list
  //only do this if this device is master or if no master is found on net
  if (netDevice.ip.toString() != "0.0.0.0" && netDevice.ip.toString() != "(IP unset)" && (myEEPROMdata.master || (masterIP.toString() == "0.0.0.0") || masterIP.toString() == "(IP unset)"))
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
//first verifies this is the myEEPROMdata.master
int fishyDevice::findNode(IPAddress lookupIP)
{
  if (myEEPROMdata.master || (masterIP.toString() == "0.0.0.0" || masterIP.toString() == "(IP unset)"))
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
//first verifies this is the myEEPROMdata.master
int fishyDevice::findDeadNode()
{

  for (int i = 0; i < MAX_DEVICE; i++)
  {
    if (deviceArray[i].dead)
    {
      return i;
    }
  }
  return -1; //no more room - oh well
  //TODO - alert the user on the webpage when the number of devices is at  MAX_NODES and no more space can be found
}

//store an updated device in the deviceArray at index
void fishyDevice::updateNode(int index, fishyDeviceData updatedDevice)
{
  updatedDevice.timeStamp = millis();
  deviceArray[index] = updatedDevice;
}

//store a new device's data in the deviceArray
int fishyDevice::storeNewNode(fishyDeviceData newDevice)
{

  int index = findDeadNode();
  if (index > -1)
  {
    newDevice.timeStamp = millis();
    deviceArray[index] = newDevice;
    return index;
  }
  else
    return -1;
}
//--------------------------------------------------------------------------

//find nodes that have dropped of the net for more than about 10 minutes and mark them as dead
void fishyDevice::cullDeadNodes()
{
  unsigned long now = millis();
  long age;
  for (int i = 0; i < MAX_DEVICE; i++)
  {
    if (!deviceArray[i].dead)
    {
      age = (long)((now - deviceArray[i].timeStamp) / 60000); //age in minutes (if negative then millis() has rolled over)
      if (DEBUG_MESSAGES)
      {
        Serial.printf("[cullDeadNodes] Node %d ", i);
        Serial.print(deviceArray[i].isMaster ? "(MASTER) " : "");
        Serial.print(deviceArray[i].name + " @ " + deviceArray[i].ip.toString());
        Serial.printf(" was updated %d minutes ago. \n", age);
      }
      if (age > 7)
      { //7 minutes off the net seems long enough to assume the node is dead
        deviceArray[i].dead = true;
        if (DEBUG_MESSAGES)
        {
          Serial.printf("[cullDeadNodes] Node %d ", i);
          Serial.print(deviceArray[i].isMaster ? "(MASTER) " : "");
          Serial.print(deviceArray[i].name + " @ " + deviceArray[i].ip.toString());
          Serial.println(" is now marked dead.");
        }
      }
    }
  }
}

//==================================================
// EEPROM Management functions
//==================================================
void fishyDevice::storeDataToEEPROM()
{
  uint addr = 0;
  if (DEBUG_MESSAGES)
  {
       Serial.println("[storeDataToEEPROM]start");
  }
  encodeDeviceCustomData(); //get the device custom data and put it into the EEPROM struct
  EEPROM.begin(1024);
  // replace values in EEPROM
  EEPROM.put(addr, myEEPROMdata);
  EEPROM.commit();
  EEPROM.end();
  if (DEBUG_MESSAGES)
  {
       Serial.println("[storeDataToEEPROM]finish");
  }
}

void fishyDevice::retrieveDataFromEEPROM()
{
  uint addr = 0;
  EEPROM.begin(1024);
  // load EEPROM data into RAM, see it
  EEPROM.get(addr, myEEPROMdata);
  initializePersonalityIfNew();
  myWifiConnect.softAP_ssid = myEEPROMdata.namestr; //update if needed
  extractDeviceCustomData();                        //get the device custom data from the EEPROM struct and move it to the EEPROMdeviceData struct
  EEPROM.end();
  storeDataToEEPROM(); //repush data to EEPROM to capture any changes or initialized data
}

bool fishyDevice::loadCredentials()
{
  EEPROM.begin(1024);
  EEPROM.get(EEPROMsz + 1, myWifiConnect.ssid);
  EEPROM.get(EEPROMsz + 1 + sizeof(myWifiConnect.ssid), myWifiConnect.password);
  char ok[2 + 1];
  EEPROM.get(EEPROMsz + 1 + sizeof(myWifiConnect.ssid) + sizeof(myWifiConnect.password), ok);
  EEPROM.end();
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.printf("Credentials size: %d, starting at %d (total): %d.\n", sizeof(myWifiConnect.ssid) + sizeof(myWifiConnect.password) + sizeof(ok), EEPROMsz, sizeof(myWifiConnect.ssid) + sizeof(myWifiConnect.password) + sizeof(ok) + EEPROMsz);
  }
  if (String(ok) != String("OK"))
  {
    myWifiConnect.ssid[0] = 0;
    myWifiConnect.password[0] = 0;
  }
  if (strlen(myWifiConnect.password) > 0 && (String(ok) == String("OK")))
  {
    if (DEBUG_WIFI_MESSAGES)
    {
      Serial.println("Recovered WiFi credentials:");
      Serial.print(myWifiConnect.ssid);
      Serial.println(": ********\n");
    }
    return true;
  }
  else
  {
    return false;
  }
}

/** Store WLAN credentials to EEPROM */
void fishyDevice::saveCredentials()
{
  EEPROM.begin(1024);
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.print("[handleWifiSave] about to save: ");
    Serial.print(myWifiConnect.ssid);
    Serial.print(" ");
    Serial.println(myWifiConnect.password);
  }
  EEPROM.put(EEPROMsz + 1, myWifiConnect.ssid);
  EEPROM.put(EEPROMsz + 1 + sizeof(myWifiConnect.ssid), myWifiConnect.password);
  char ok[2 + 1] = "OK";
  EEPROM.put(EEPROMsz + 1 + sizeof(myWifiConnect.ssid) + sizeof(myWifiConnect.password), ok);
  EEPROM.commit();
  EEPROM.end();
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.println("\nNew WiFi credentials saved.");
    loadCredentials();
  }
}

//================================================================================
// wifi and webserver and websock functions
//================================================================================

//------------------------------------------------------------------------------
//--------------------------------------- WiFi----------------------------------
// -----------------------------------------------------------------------------
void fishyDevice::WiFiSetup()
{
  gotIpEventHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event)
  {
    Serial.print("[WiFiSetup - getIP event handler] Connected, IP: ");
    Serial.println(WiFi.localIP());
    runNormalServer();
  });

  myWifiConnect.connect = false;
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.println("\n---------------------\n[WiFiSetup] Configuring wifi...");
  }
  bool result = loadCredentials();

  if (!result)
  {
    if (DEBUG_WIFI_MESSAGES)
    {
      Serial.println("No WiFi credentials found.  Going into Soft AP mode to accept new WiFi SSID and Password.");
    }
    myWifiConnect.connect = false;
    myWifiConnect.softAPmode = true;
    runSoftAPServer(); //make softAP and serve wifi ssid pass collection page
  }
  else
  {
    if (DEBUG_WIFI_MESSAGES)
    {
      Serial.println("WiFi credentials loaded.  Going to try and connect.");
    }
    myWifiConnect.connect = true;
    myWifiConnect.softAPmode = false;
  }
}

void fishyDevice::connectWifi()
{
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.print("Connecting as WiFi client...Try number: ");
    Serial.println(myWifiConnect.connectTryCount);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(myWifiConnect.ssid, myWifiConnect.password);
  
  int connRes = WiFi.waitForConnectResult();
  //delay(500);
  //int connRes = WiFi.status();

  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.print("\nConnection Try Result: ");
    Serial.println(readStatus(connRes));
  }

  if (connRes != WL_CONNECTED)
  {
    myWifiConnect.connect = true;
  }
  else
  {
    myWifiConnect.connect = false;

//    runNormalServer();
  }
}

//a wifi connection status response translator
String fishyDevice::readStatus(int s)
{
  if (s == WL_NO_SHIELD)
    return String("WL_NO_SHIELD");
  if (s == WL_IDLE_STATUS)
    return String("WL_IDLE_STATUS");
  if (s == WL_NO_SSID_AVAIL)
    return String("WL_NO_SSID_AVAIL");
  if (s == WL_SCAN_COMPLETED)
    return String("WL_SCAN_COMPLETED");
  if (s == WL_CONNECTED)
    return String("WL_CONNECTED");
  if (s == WL_CONNECT_FAILED)
    return String("WL_CONNECT_FAILED");
  if (s == WL_CONNECTION_LOST)
    return String("WL_CONNECTION_LOST");
  if (s == WL_DISCONNECTED)
    return String("WL_DISCONNECTED");
}
// this manages trying to connect to the network
void fishyDevice::manageConnection()
{

  static long lastConnectTry = 0;
  int s = WiFi.status();

  //if not connected give it a try every 10 seconds
  if (s != WL_CONNECTED && (millis() > (lastConnectTry + 10000) || (myWifiConnect.connectTryCount == 0)))
  {
    myWifiConnect.connect = false;
    if (DEBUG_WIFI_MESSAGES)
    {
      Serial.println("Connecting...");
    }
    lastConnectTry = millis();
    if(myEEPROMdata.blink_Enable){
      fastBlinks(myWifiConnect.connectTryCount);
    }
    connectWifi();
    myWifiConnect.connectTryCount = myWifiConnect.connectTryCount + 1;
    if (myWifiConnect.connectTryCount > 3)
    {
      myWifiConnect.softAPmode = true;
      runSoftAPServer();
    }
  }
  // Detect a WLAN status change
  if (myWifiConnect.status != s)
  {
    if (DEBUG_WIFI_MESSAGES)
    {
      Serial.print("Status: ");
      Serial.println(readStatus(s));
    }
    myWifiConnect.status = s;
    if (s == WL_CONNECTED)
    {
      runNormalServer();
    }
    else if (s == WL_NO_SSID_AVAIL)
    {
      WiFi.disconnect();
    }
    else if (s == 4)
    {
      WiFi.disconnect();
    }
  }
}

//-----------------------------------------------------------------------------
//----------------------------Websock Functions--------------------------------
//-----------------------------------------------------------------------------

void fishyDevice::webSocketEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (DEBUG_MESSAGES)
  {
    Serial.printf("webSocketEventHandler(%d, %d, ...)\r\n", client->id(), type);
  }
  switch (type)
  {
  case WS_EVT_DISCONNECT:
  {
    if (DEBUG_MESSAGES)
    {
      Serial.printf("[%u] Disconnected!\r\n", client->id());
    }
    client->text("DISCONNECTED", strlen("DISCONNECTED"));
  }
  break;
  case WS_EVT_CONNECT:
  {
    //on connect provide this node's information to all clients
    webSocket->textAll(getNodeJSON().c_str(), strlen(getNodeJSON().c_str()));
    if (DEBUG_MESSAGES)
    {
      Serial.printf("WebSocket [%s][%u] connect\n", server->url(), client->id());
    }
  }
  break;
  case WS_EVT_DATA:
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      if (DEBUG_MESSAGES)
      {
        Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
      }
      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      if (DEBUG_MESSAGES)
      {
        Serial.printf("%s\n", msg.c_str());
      }

      if (info->opcode == WS_TEXT)
      {
        executeCommands((char *)msg.c_str(), WiFi.localIP(),client->id());
        // repeat back received message data to all connected clients
        webSocket->textAll(msg.c_str());
      }
      else
      {
        //Not needed for fishyDevice - handle binary if desired
      }
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          if (DEBUG_MESSAGES)
          {
            Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
            Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
          }
      }

      if (DEBUG_MESSAGES)
      {
        Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
      }

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      if (DEBUG_MESSAGES)
      {
        Serial.printf("%s\n", msg.c_str());
      }
      if ((info->index + len) == info->len)
      {
        if (DEBUG_MESSAGES)
        {
          Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        }
        if (info->final)
        {
          if (DEBUG_MESSAGES)
          {
            Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          }
          if (info->message_opcode == WS_TEXT)
          {
            executeCommands((char *)msg.c_str(), WiFi.localIP(),client->id());
            // send data to client
            client->text(msg.c_str());
          }
          else
          {
            //  LATER - if desired add binary support
            //	client->binary("I got your binary message");
          }
        }
      }
    }
  }
  break;
  //  LATER - if desired add binary support
  //case WStype_BIN:
  //	if (DEBUG_MESSAGES){Serial.printf("[%u] get binary length: %u\r\n", client->id(), length);}
  //	hexdump(payload, length);

  //	// echo data back to browser
  //	webSocket->sendBIN(client->id(), payload, length);
  //break;
  default:
    if (DEBUG_MESSAGES)
    {
      Serial.printf("Invalid WStype [%d]\r\n", type);
    }
    break;
  }
}
//send an update any connected websocket clients to update the screen.
//it only provides updates every 500 mSec unless forceUpdate = true.
//NOTE: forceUpdate=true will casue performance problems if called too frequently
//also sends a notice to MASTER via UDP to report a command execution if forceUpdate is true (not used for rapid state changes)
void fishyDevice::updateClients(String message)
{
  updateClients(message, false);
}
void fishyDevice::updateClients(String message, bool forceUpdate)
{
  static unsigned long lastUpdate = millis();
  String text = "";
  if ((millis() - lastUpdate > 500) || forceUpdate)
  {
    String nodeJSON = getNodeJSON();
    text = "MSG:" + message + "~*~*DAT:" + nodeJSON;

    lastUpdate = millis();
    UDPpollReply(WiFi.localIP());
    if (DEBUG_MESSAGES){Serial.println(text);}
    webSocket->textAll(text.c_str());
    
    if (forceUpdate)
    {
      String response = UDPmakeActivityMessage(message);
      if (masterIP.toString() != "0.0.0.0" && masterIP.toString() != "(IP unset)")
      {
        Udp.beginPacket(masterIP, UDP_LOCAL_PORT);
        Udp.write(response.c_str());
        Udp.endPacket();
      }else if(myEEPROMdata.master){ //send logged events from the maser device itself
        if (loggerIP.toString() != "0.0.0.0" && loggerIP.toString() != "(IP unset)")
          {
           char strForParse[MAXCMDSZ*2] = "";
           strncpy(strForParse, response.c_str(), MAXCMDSZ*2);
           UDPhandleActivityMessage(strForParse,WiFi.localIP()); 
          }
      }
    }
  }
}
//similar to above but just send a message to the client_id via websocket
void fishyDevice::updateSpecificClient(String message, int client_id)
{
    String text = "MSG:" + message + "~*~*DAT:" + getNodeJSON();;
    if (DEBUG_MESSAGES){Serial.println(text);}
    webSocket->text(client_id,text.c_str());
}

//helper function to format an activity message with or without the ~UDP label
String fishyDevice::UDPmakeActivityMessage(String message){
        String str;
        str = UDP_ACTIVITY; 
        str += ":device=";
        str += WiFi.localIP().toString();
        str += ";message=";
        str += message;
        str += ";nodeStatus=";
        str += getStatusString();
        str += ";nodeShortStatus=";
        str += getShortStatString();
        str += ";";
        return str;
}

//-----------------------------------------------------------------------------
//-------------------------Web Server Functions--------------------------------
//-----------------------------------------------------------------------------

//Web Server - provide a JSON structure with all the deviceArray data
void fishyDevice::handleNetworkJSON(AsyncWebServerRequest *request)
{
  if (myEEPROMdata.master || (masterIP.toString() == "0.0.0.0" || masterIP.toString() == "(IP unset)"))
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
void fishyDevice::handleNodeJSON(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", getNodeJSON().c_str());
}
//Web Server - provide a configuration string that when sent back to device via websock will update its configurataion
void fishyDevice::handleConfigString(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", getConfigString().c_str());
}
//When file isn't found or root is called for non-master this shows a link to the master.
//Also provides link to this device's control panel.
void fishyDevice::handleNotMaster(AsyncWebServerRequest *request)
{
  String ipToShow = masterIP.toString();
  if (ipToShow == "0.0.0.0" || ipToShow == "(IP unset)")
  {
    ipToShow = WiFi.localIP().toString();
  }
  String response = " <!DOCTYPE html> <html> <head> <title>fishDIY Device Network</title> <style> body {background-color: #edf3ff;font-family: \"Lucida Sans Unicode\", \"Lucida Grande\", sans-serif;}  .fishyHdr {align:center; border-radius:25px;font-size: 18px; font-weight: bold;color: white;background: #3366cc; vertical-align: middle; } </style>  <body> <div class=\"fishyHdr\">Not Master</div><p class='configPanel'>Go to <a href=\"http://" + ipToShow + "\"> Master (" + ipToShow + ")</a> for a list of devices.<br>Or go directly to this device's <a href=\"http://" + WiFi.localIP().toString() + "/control\"> control panel (http://" + WiFi.localIP().toString() + "/control)</a></p><div class='fishyFtrSw'></div></body> </html> ";
  request->send(200, "text/html", response.c_str());
}

//return the JSON data for the entire network (limited data for each live node)
String fishyDevice::getNetworkJSON()
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
      // put fishyDeviceData data in a string for all network nodes
      // Note - this is string will be parsed by scripts in webresources.h
      // and is paralleled by UDPpollReply and if configuration setting data updated by the website then UDPparseConfigResponse is affected;
      // if adding data elements all these may need updating.This function sends data as follows (keep this list updated):
      // {ip,name,typestr,statusstr,inError,isMaster,shortStat,locationX (X),locationY (Y),locationZ(Z),dead}
      temp += "{\"ip\":\"" + deviceArray[i].ip.toString() + "\",\"name\":\"" + String(deviceArray[i].name) + "\",\"typestr\":\"" + String(deviceArray[i].typestr) + "\",\"statusstr\":\"" + String(deviceArray[i].statusstr) + "\",\"inError\":\"" + String(deviceArray[i].inError ? "true" : "false") + "\",\"isMaster\":\"" + String(deviceArray[i].isMaster ? "true" : "false") + "\",\"shortStat\":\"" + String(deviceArray[i].shortStat) + "\",\"X\":\"" + String(deviceArray[i].locationX) + "\",\"Y\":\"" + String(deviceArray[i].locationY) + "\",\"Z\":\"" + String(deviceArray[i].locationZ) + "\",\"dead\":\"" + String(deviceArray[i].dead) + "\"}";
    }
  }
  temp += "],\"logger\":\""+loggerIP.toString()+"\"}";

  return temp;
}

//return the JSON data for this device from the device specific .ino file
// put this fishyDeviceData data in a string.
// Note - this is string will be parsed by scripts in webresources.h
// and is paralleled by UDPpollReply and if configuration setting data updated by the website then UDPparseConfigResponse is affected;
String fishyDevice::getNodeJSON()
{
  return getDeviceSpecificJSON();
}

//this creates the iframes for all the devices in the network, if /SWupdater then it loads those forms, otherwise it loads /ctrlPanels for each iframe
void fishyDevice::handleRoot(AsyncWebServerRequest *request)
{
  if (myEEPROMdata.master || (masterIP.toString() == "0.0.0.0" || masterIP.toString() == "(IP unset)"))
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("\n[handleRoot]\n");
      Serial.println(FPSTR(WEBROOTSTR));
    }
    request->send_P(200, "text/html", WEBROOTSTR);
  }
  else
  {
    if (DEBUG_MESSAGES)
    {
      Serial.println("\n[handleRoot] Not Master, showing link to Master.");
    }
    handleNotMaster(request);
  }
}

void fishyDevice::handleWifiUpdater(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleWifiUpdater] Start");
  }
  String n = "";
  String p = "";
  if (request->hasParam("n", true))
  {
    n = request->arg("n");
  }
  if (request->hasParam("p", true))
  {
    p = request->arg("p");
  }

  String responseString = "<!doctypehtml><title>fishDIY Device Network</title><meta content=\"width=device-width,initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css rel=stylesheet id=styles><link href=styles.css rel=stylesheet><script src=CommonWebFunctions.js></script><div class=main id=myBody><script>var pass=encodeURIComponent(\"" + p + "\"),ssid=encodeURIComponent(\"" + n + "\");function addDevice(e){var a=\"<div class='CPdevice' id='CP-\"+e.ip+\"'>\";return a+=addInnerDevice(e),a+=\"</div>\"}function addWIFIMASTERDevice(){return\"<div class='CPhd' id='hd1-master'>Master Update</div>\",\"<form class='swUpdate' method='POST' action='WIFIupdater' ><h3>Prefill WIFI Credentials for All Devices:</h3><input type='text' placeholder='network' name='n' /><br /><input type='password' placeholder='password' name='p' '/><br /><input type='submit' value='Submit'/></form>\",\"<div class='CPft' id='CPft-master'></div>\",\"</div>\",\"<div class='CPdevice' style='background-color: #cce6ff;' id='CP-MASTER'><div class='CPhd' id='hd1-master'>Master Update</div><form class='swUpdate' method='POST' action='WIFIupdater' ><h2>Prefill WIFI Credentials for All Devices:</h2><input type='text' placeholder='network' name='n' /><br /><input type='password' placeholder='password' name='p' '/><br /><input type='submit' value='Submit'/></form><div class='CPft' id='CPft-master'></div></div>\"}function addInnerDevice(e){var a;a=\"true\"==e.isMaster?\"MASTER:\"+e.ip:e.ip;var i=\"<div class='CPhd' id='hd1-\"+e.ip+\"'>\"+e.name+\"</div>\";return i+=\"<iframe id='wifiUpdate-\"+e.ip+\"' class='swUpdate' src='http://\"+e.ip+\"/wifi?n=\"+ssid+\"&p=\"+pass+\"'></iframe><br>\",i+=\"<div class='CPft' id='CPft-\"+e.ip+\"'>\"+a+\"</div>\"}function buildPage(){alertBadBrowser();var a,i,e=\"./network.JSON?nocache=\"+(new Date).getTime(),d=[],t=_(\"myBody\"),r=document.createElement(\"DIV\");r.className=\"fishyHdr\",t.appendChild(r),r.innerHTML=\"fishyDevice WIFI Credentials Update\",(r=document.createElement(\"DIV\")).className=\"CPhd3\",t.appendChild(r),(i=document.createElement(\"DIV\")).className=\"fishyFtr\",t.appendChild(i),i.innerHTML=\"<a href='/'>[Controls]</a>  <a href='/SWupdater'>[SW Update]</a>  <a href='/WIFIupdater'>[WIFI Update]</a>\",loadJSON(e,function(e){a=e.fishyDevices,d.push(addWIFIMASTERDevice()),Array.prototype.forEach.call(a,function(e,a){d.push(addDevice(e))}),(r=document.createElement(\"DIV\")).className=\"flex-container\",r.id=\"flex-container\",r.innerHTML=d.join(\"\"),t.insertBefore(r,i)})}window.onload=buildPage()</script></div>";

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", responseString);

  //response->addHeader("Location", "wifi", true);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);
}

// Wifi config page
void fishyDevice::handleWifi(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleWifi] wifi");
  }

  String webpage = "<!doctypehtml><meta content=\"width=device-width,initial-scale=1\"name=viewport><style>:root{font-family: Arial, Helvetica, sans-serif;font-style: normal;}</style><script>function findGetParameter(t){var e=null,o=[];return location.search.substr(1).split(\"&\").forEach(function(n){(o=n.split(\"=\"))[0]===t&&(e=decodeURIComponent(o[1]))}),e}</script><form action=wifisave method=POST><h4>Connect to WIFI Network:</h4><input id=ssid name=n placeholder=network><br><input type=password id=pwd name=p placeholder=password><br><input type=submit value=\"Submit New WIFI Credentials\"><br>(Submitting blank info will delete saved credentials)</form><br><hr><form action=justreboot method=POST><h4>Reboot Device:</h4>Reboot and try to make the device reconnect using exisitng WIFI credentials:<br><input type=submit value=\"Reboot Device\"></form><script>function doGET(){var e=findGetParameter(\"n\"),d=findGetParameter(\"p\");document.getElementById(\"ssid\").value=e,document.getElementById(\"pwd\").value=d}window.onload=doGET()</script>";

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", webpage.c_str());

  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);
}

void fishyDevice::handleWifiSave(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleWifiSave] wifi save");
  }
  if (request->hasParam("n", true))
  {
    String n = request->arg("n");
    n.toCharArray(myWifiConnect.ssid, sizeof(myWifiConnect.ssid) - 1);
  }
  else
  {
    //do something if we don't find the SSID arg - ignored for now
    if (DEBUG_MESSAGES)
    {
      Serial.println("[handleWifiSave] no SSID found");
    }
  }
  if (request->hasParam("p", true))
  {
    String p = request->arg("p");
    p.toCharArray(myWifiConnect.password, sizeof(myWifiConnect.password) - 1);
  }
  else
  {
    //do something if we don't find the password arg - ignored for now
    if (DEBUG_MESSAGES)
    {
      Serial.println("[handleWifiSave] no password found");
    }
  }
  if ((myWifiConnect.ssid == ""))
  {
    resetWiFiCredentials();
  } //if SSID is blank, reset credentials

  String responseString = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>:root{font-family: Arial, Helvetica, sans-serif;font-style: normal;}</style></head><body><h1>Configuration updated.</h1>You will now be disconnected from this device and need to reconnect to your normal WiFi network.<br><br>If the WiFi credentials work it will reboot and connect to your network. If they fail, it will return to Access Point mode after ~1 minute of trying. <br><br>If that occurs, use your WiFi settings to find \"" + String(myWifiConnect.softAP_ssid) + "\"). Then return to \"" + apIP.toString() + "/wifi\" to reenter the network ID and password.<br><br>Note: The LED on the device blinks slowly when acting as an Access Point.<hr><a href=\"/wifi\">Return to the WiFi settings page.</a></body></html>";

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", responseString);

  //response->addHeader("Location", "wifi", true);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);

  saveCredentials();
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleWifiSave] save complete");
  }
  resetOnNextLoop = true;
}

void fishyDevice::handleJustReboot(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleJustReboot] just reboot");
  }

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body><h1>Rebooting Device.</h1><hr><a href=\"/wifi\">Return to the WiFi settings page.</a></body></html></body></html>");

  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);

  resetOnNextLoop = true;
}

void fishyDevice::handleCtrl(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("\n[handleCtrl]\n");
    Serial.println(FPSTR(webctrlstrPtr));
  }
  request->send_P(200, "text/html", webctrlstrPtr);
}

//show the SW update form for the specifc device (function for every device webserver)
void fishyDevice::handleSWupdateDevForm(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("[handleSWupdateDevForm]");
  }
  
  //build the device info string
  String SwUpdate_Device_Info = "Type: " + String(myEEPROMdata.typestr) + "<br>Software Version:" + String(myEEPROMdata.swVer) + "<br>Initialization String:" + String(myEEPROMdata.initstr) + "<br>Current SW Size: " + String(ESP.getSketchSize()) + "<br>Free Space: " + String(ESP.getFreeSketchSpace());
  if(ESP.getFreeSketchSpace()<ESP.getSketchSize()){
    SwUpdate_Device_Info = SwUpdate_Device_Info  + String("<BR><i>WARNING: Device needs free space bigger than the sketch you uploading. Likely not enough is available.  If it fails you will need to update via serial connection.</i>");
  }
  SwUpdate_Device_Info = SwUpdate_Device_Info  + "<script>var SWDELAY=" + String(SW_UPDATE_DELAY) + ";</script>";

  String responseStr;
  responseStr = String(FPSTR(WEBSTR_SWUPDATE_PT1)) + String(SwUpdate_Device_Info) + String(FPSTR(WEBSTR_SWUPDATE_PT2));
  if (DEBUG_MESSAGES)
  {
    Serial.println(responseStr);
  }
  request->send(200, "text/html", responseStr.c_str());
}

//show the CSS (function for every device webserver)
void fishyDevice::handleCSS(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("\n[handleCSS]\n");
    Serial.println(FPSTR(WEBSTYLESSTR));
  }
  request->send_P(200, "text/css", WEBSTYLESSTR);
}

//show the SW update form for the specifc device (function for every device webserver)
void fishyDevice::handleJS(AsyncWebServerRequest *request)
{
  if (DEBUG_MESSAGES)
  {
    Serial.println("\n[handleJS]\n");
    Serial.println(FPSTR(WEBSTR_COMMON_JS));
  }
  request->send_P(200, "application/javascript", WEBSTR_COMMON_JS);
}

void fishyDevice::handleSWupdateDevPost(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    if (DEBUG_MESSAGES) {
      Serial.printf("Free Space: %d\nCurrent sketch size: %d\n", ESP.getFreeSketchSpace(),ESP.getSketchSize());
      Serial.printf("Chip Size (from SDK): %d\nActual Chip Size (from ID): %d\n", ESP.getFlashChipSize(),ESP.getFlashChipRealSize());
      Serial.printf("Update Start: %s\n", filename.c_str());
      
    }
    Update.runAsync(true);
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
    {
      if (DEBUG_MESSAGES){Update.printError(Serial);}
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      if (DEBUG_MESSAGES){Update.printError(Serial);}
    }
  }
  if (final)
  {
    if (Update.end(true))
    {
      if (DEBUG_MESSAGES){Serial.printf("Update Successful: %uB\n", index + len);}
    }
    else
    {
      if (DEBUG_MESSAGES){Update.printError(Serial);}
    }
  }
}

void fishyDevice::handleSWupdateDevPostDone(AsyncWebServerRequest *request)
{
  if (Update.hasError())
  {
    request->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
  }
  else
  {
    if (DEBUG_MESSAGES){Serial.println("Got into Update Post Done. Delay and Restarting.");}
    delay(100);
    ESP.restart();
  }
}

//web server response to other
void fishyDevice::handleNotFound(AsyncWebServerRequest *request)
{
  String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
  if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body))
    return;
  handleNotMaster(request);
}

//wipe out the wifi credentials
void fishyDevice::resetWiFiCredentials()
{
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.println("[resetWiFiCredentials] Resetting Credentials");
  }
  WiFi.disconnect();
  myWifiConnect.ssid[0] = '\0';
  myWifiConnect.password[0] = '\0';
  saveCredentials();
  resetOnNextLoop = true;
}

//function to setup normal functioning webpages and other net protocals after connecting to wifi
void fishyDevice::runNormalServer()
{
  httpServer->reset();
  if (dnsServer != NULL)
  {
    dnsServer->stop();
  }
  WiFi.setAutoReconnect(true);
  Udp.begin(UDP_LOCAL_PORT); //start listening on UDP port for node-node traffic
  UDPbroadcast();            //get a poll going
  if (DEBUG_UDP_MESSAGES)
  {
    Serial.println("[runNormalServer] Initiated UDP Broadcast to poll for other devices and a MASTER.");
  }
  webSocket = new AsyncWebSocket("/ws");
  webSocket->onEvent(std::bind(&fishyDevice::webSocketEventHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

  if (DEBUG_MESSAGES)
  {
    Serial.print("\n--------------------------\nConnected to SSID: ");
    Serial.println(myWifiConnect.ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  if(myEEPROMdata.blink_Enable){
    fastBlinks(5);
  }
  String hostName;
  if (myEEPROMdata.master)
  {
    hostName = "fishyDIY";
  }
  else
  {
    hostName = "fishyDIYNode" + String(WiFi.localIP()[3]);
  }

  MDNS.begin(hostName.c_str()); //start mDNS to fishyDIY.local

  httpServer->on("/", HTTP_ANY, std::bind(&fishyDevice::handleRoot, this, std::placeholders::_1));
  httpServer->on("/SWupdater", HTTP_GET, std::bind(&fishyDevice::handleRoot, this, std::placeholders::_1));
  httpServer->on("/WIFIupdater", HTTP_ANY, std::bind(&fishyDevice::handleWifiUpdater, this, std::placeholders::_1));
  httpServer->on("/control", HTTP_GET, std::bind(&fishyDevice::handleCtrl, this, std::placeholders::_1));
  httpServer->on("/SWupdateDevForm", HTTP_GET, std::bind(&fishyDevice::handleSWupdateDevForm, this, std::placeholders::_1));
  httpServer->on("/SWupdateGetForm", HTTP_GET, std::bind(&fishyDevice::handleSWupdateDevForm, this, std::placeholders::_1));
  httpServer->on("/SWupdatePostForm", HTTP_POST, std::bind(&fishyDevice::handleSWupdateDevPostDone, this, std::placeholders::_1), std::bind(&fishyDevice::handleSWupdateDevPost, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  httpServer->on("/network.JSON", HTTP_GET, std::bind(&fishyDevice::handleNetworkJSON, this, std::placeholders::_1));
  httpServer->on("/node.JSON", HTTP_GET, std::bind(&fishyDevice::handleNodeJSON, this, std::placeholders::_1));
  httpServer->on("/config", HTTP_GET, std::bind(&fishyDevice::handleConfigString, this, std::placeholders::_1));
  httpServer->on("/styles.css", HTTP_GET, std::bind(&fishyDevice::handleCSS, this, std::placeholders::_1));
  httpServer->on("/CommonWebFunctions.js", HTTP_GET, std::bind(&fishyDevice::handleJS, this, std::placeholders::_1));
  httpServer->on("/wifi", HTTP_ANY, std::bind(&fishyDevice::handleWifi, this, std::placeholders::_1));
  httpServer->on("/wifisave", HTTP_POST, std::bind(&fishyDevice::handleWifiSave, this, std::placeholders::_1));
  httpServer->on("/justreboot", HTTP_POST, std::bind(&fishyDevice::handleJustReboot, this, std::placeholders::_1));
  httpServer->onNotFound(std::bind(&fishyDevice::handleNotFound, this, std::placeholders::_1));
  httpServer->onRequestBody(std::bind(&fishyDevice::handleOnRequestBody, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  httpServer->addHandler(webSocket);
  httpServer->begin();

  MDNS.addService("http", "tcp", 80);

  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.println("HTTP server started\n--------------------------\n");
  }
  announceReadyOnUDP();
}

void fishyDevice::handleOnRequestBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data)))
    return;
};

//function to setup AP mode webpages (for WiFi setting only)
void fishyDevice::runSoftAPServer()
{
  httpServer->reset();
  dnsServer = new DNSServer();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(myWifiConnect.softAP_ssid, myWifiConnect.softAP_password);
  delay(500); // Without delay I've seen the IP address blank

  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

  if (DEBUG_MESSAGES)
  {
    Serial.print("\n--------------------------\n[runSoftAPServer]AP Name to Connect via WiFi: ");
    Serial.println(myWifiConnect.softAP_ssid);
    Serial.print("\AP Network Password: ");
    Serial.println(myWifiConnect.softAP_password);
    Serial.print("\AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }

  httpServer->on("/", HTTP_GET, std::bind(&fishyDevice::handleWifi, this, std::placeholders::_1));
  httpServer->on("/wifi", HTTP_GET, std::bind(&fishyDevice::handleWifi, this, std::placeholders::_1));
  httpServer->onNotFound(std::bind(&fishyDevice::handleWifi, this, std::placeholders::_1));
  httpServer->on("/wifisave", HTTP_POST, std::bind(&fishyDevice::handleWifiSave, this, std::placeholders::_1));
  httpServer->on("/justreboot", HTTP_POST, std::bind(&fishyDevice::handleJustReboot, this, std::placeholders::_1));
  httpServer->begin(); // Web server start
  if (DEBUG_WIFI_MESSAGES)
  {
    Serial.println("HTTP server started\n--------------------------");
  }
  if(myEEPROMdata.blink_Enable)
  {
    slowBlinks(60); //blink slowly for a few minutes
  }
}

void fishyDevice::checkWifiStatus()
{
  static long lastTimeForMessage = 0;
  if (myWifiConnect.softAPmode == true)
  {
    dnsServer->processNextRequest();
  }
  if (DEBUG_WIFI_MESSAGES)
  {
    if (millis() > lastTimeForMessage + 4000)
    {
      int s = WiFi.status();
      lastTimeForMessage = millis();
      Serial.print("Status");
      Serial.println(readStatus(s));
      Serial.print("myWifiConnect.status: ");
      Serial.println(readStatus(myWifiConnect.status));
      Serial.print("myWifiConnect.connect: ");
      Serial.println(myWifiConnect.connect ? "true" : "false");
      Serial.print("myWifiConnect.softAPmode: ");
      Serial.println(myWifiConnect.softAPmode ? "true" : "false");
    }
  }
}


/*
FAUXMO ESP

NOTE: THIS REPOSITORY IS NO LONGER MAINTAINED SO BUILDING FAUXMO THIS INTO FISHYDEVICE CODE

Copyright (C) 2016-2020 by Xose Pérez <xose dot perez at gmail dot com>

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


// -----------------------------------------------------------------------------
// UDP
// -----------------------------------------------------------------------------

void fauxmoESP::_sendUDPResponse() {

	DEBUG_MSG_FAUXMO("[FAUXMO] Responding to M-SEARCH request\n");

	IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
	mac = String("FD-") + mac;

	char response[strlen(FAUXMO_UDP_RESPONSE_TEMPLATE) + 128];
    snprintf_P(
        response, sizeof(response),
        FAUXMO_UDP_RESPONSE_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3],
		_tcp_port,
        mac.c_str(), mac.c_str()
    );

	#if DEBUG_FAUXMO_VERBOSE_UDP
    	DEBUG_MSG_FAUXMO("[FAUXMO] UDP response sent to %s:%d\n%s", _udp.remoteIP().toString().c_str(), _udp.remotePort(), response);
	#endif

    _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
	#if defined(ESP32)
	    _udp.printf(response);
	#else
	    _udp.write(response);
	#endif
    _udp.endPacket();

}

void fauxmoESP::_handleUDP() {

	int len = _udp.parsePacket();
    if (len > 0) {

		unsigned char data[len+1];
        _udp.read(data, len);
        data[len] = 0;

		#if DEBUG_FAUXMO_VERBOSE_UDP
			DEBUG_MSG_FAUXMO("[FAUXMO] UDP packet received\n%s", (const char *) data);
		#endif

        String request = (const char *) data;
        if (request.indexOf("M-SEARCH") >= 0) {
            if ((request.indexOf("ssdp:discover") > 0) || (request.indexOf("upnp:rootdevice") > 0) || (request.indexOf("device:basic:1") > 0)) {
                _sendUDPResponse();
            }
        }
    }

}


// -----------------------------------------------------------------------------
// TCP
// -----------------------------------------------------------------------------

void fauxmoESP::_sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime) {

	char headers[strlen_P(FAUXMO_TCP_HEADERS) + 32];
	snprintf_P(
		headers, sizeof(headers),
		FAUXMO_TCP_HEADERS,
		code, mime, strlen(body)
	);

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] Response:\n%s%s\n", headers, body);
	#endif

	client->write(headers);
	client->write(body);

}

String fauxmoESP::_deviceJson(unsigned char id) {

	if (id >= _devices.size()) return "{}";

	String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
	mac = String("FD-") + mac;

	fauxmoesp_device_t device = _devices[id];
    char buffer[strlen_P(FAUXMO_DEVICE_JSON_TEMPLATE) + 64];
    snprintf_P(
        buffer, sizeof(buffer),
        FAUXMO_DEVICE_JSON_TEMPLATE,
        device.name, mac.c_str(), id+1,
        device.state ? "true": "false",
        device.value
    );

	return String(buffer);

}

bool fauxmoESP::_onTCPDescription(AsyncClient *client, String url, String body) {

	(void) url;
	(void) body;

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling /description.xml request\n");

	IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
	mac = String("FD-") + mac;

	char response[strlen_P(FAUXMO_DESCRIPTION_TEMPLATE) + 64];
    snprintf_P(
        response, sizeof(response),
        FAUXMO_DESCRIPTION_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        mac.c_str(), mac.c_str()
    );

	_sendTCPResponse(client, "200 OK", response, "text/xml");

	return true;

}

bool fauxmoESP::_onTCPList(AsyncClient *client, String url, String body) {

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling list request\n");

	// Get the index
	int pos = url.indexOf("lights");
	if (-1 == pos) return false;

	// Get the id
	unsigned char id = url.substring(pos+7).toInt();

	// This will hold the response string	
	String response;

	// Client is requesting all devices
	if (0 == id) {

		response += "{";
		for (unsigned char i=0; i< _devices.size(); i++) {
			if (i>0) response += ",";
			response += "\"" + String(i+1) + "\":" + _deviceJson(i);
		}
		response += "}";

	// Client is requesting a single device
	} else {
		response = _deviceJson(id-1);
	}

	_sendTCPResponse(client, "200 OK", (char *) response.c_str(), "application/json");
	
	return true;

}

bool fauxmoESP::_onTCPControl(AsyncClient *client, String url, String body) {

	// "devicetype" request
	//TODO - use USERNAME?  See what is done by habridge
	if (body.indexOf("devicetype") > 0) {
		DEBUG_MSG_FAUXMO("[FAUXMO] Handling devicetype request\n");
		_sendTCPResponse(client, "200 OK", (char *) "[{\"success\":{\"username\": \"2WLEDHardQrI3WHYTHoMcXHgEspsM8ZZRpSKtBQr\"}}]", "application/json");
		return true;
	}

	// "state" request
	if ((url.indexOf("state") > 0) && (body.length() > 0)) {

		// Get the index
		int pos = url.indexOf("lights");
		if (-1 == pos) return false;

		DEBUG_MSG_FAUXMO("[FAUXMO] Handling state request\n");

		// Get the index
		unsigned char id = url.substring(pos+7).toInt();
		DEBUG_MSG_FAUXMO("[FAUXMO] url: %s\nID:%d\n", url.c_str(), id);
		if (id > 0) {

			--id;

			// Brightness
			pos = body.indexOf("bri");
			if (pos > 0) {
				unsigned char value = body.substring(pos+5).toInt();
				_devices[id].value = value;
				_devices[id].state = (value > 0);
			} else if (body.indexOf("false") > 0) {
				_devices[id].state = false;
			} else {
				_devices[id].state = true;
				if (0 == _devices[id].value) _devices[id].value = 255;
			}

			char response[strlen_P(FAUXMO_TCP_STATE_RESPONSE)+10];
			snprintf_P(
				response, sizeof(response),
				FAUXMO_TCP_STATE_RESPONSE,
				id+1, _devices[id].state ? "true" : "false", id+1, _devices[id].value
			);
			_sendTCPResponse(client, "200 OK", response, "text/xml");

			if (_setCallback) {
				_setCallback(id, _devices[id].name, _devices[id].state, _devices[id].value);
			}

			return true;

		}

	}

	return false;
	
}

bool fauxmoESP::_onTCPRequest(AsyncClient *client, bool isGet, String url, String body) {

    if (!_enabled) return false;

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] isGet: %s\n", isGet ? "true" : "false");
		DEBUG_MSG_FAUXMO("[FAUXMO] URL: %s\n", url.c_str());
		if (!isGet) DEBUG_MSG_FAUXMO("[FAUXMO] Body:\n%s\n", body.c_str());
	#endif

	if (url.equals("/description.xml")) {
        return _onTCPDescription(client, url, body);
    }

	if (url.startsWith("/api")) {
		if (isGet) {
			return _onTCPList(client, url, body);
		} else {
       		return _onTCPControl(client, url, body);
		}
	}

	return false;

}

bool fauxmoESP::_onTCPData(AsyncClient *client, void *data, size_t len) {

    if (!_enabled) return false;

	char * p = (char *) data;
	p[len] = 0;

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] TCP request\n%s\n", p);
	#endif

	// Method is the first word of the request
	char * method = p;

	while (*p != ' ') p++;
	*p = 0;
	p++;
	
	// Split word and flag start of url
	char * url = p;

	// Find next space
	while (*p != ' ') p++;
	*p = 0;
	p++;

	// Find double line feed
	unsigned char c = 0;
	while ((*p != 0) && (c < 2)) {
		if (*p != '\r') {
			c = (*p == '\n') ? c + 1 : 0;
		}
		p++;
	}
	char * body = p;

	bool isGet = (strncmp(method, "GET", 3) == 0);

	return _onTCPRequest(client, isGet, url, body);

}

void fauxmoESP::_onTCPClient(AsyncClient *client) {

	if (_enabled) {

	    for (unsigned char i = 0; i < FAUXMO_TCP_MAX_CLIENTS; i++) {

	        if (!_tcpClients[i] || !_tcpClients[i]->connected()) {

	            _tcpClients[i] = client;

	            client->onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {
	            }, 0);

	            client->onData([this, i](void *s, AsyncClient *c, void *data, size_t len) {
	                _onTCPData(c, data, len);
	            }, 0);

	            client->onDisconnect([this, i](void *s, AsyncClient *c) {
	                _tcpClients[i]->free();
	                _tcpClients[i] = NULL;
	                delete c;
	                DEBUG_MSG_FAUXMO("[FAUXMO] Client #%d disconnected\n", i);
	            }, 0);

	            client->onError([i](void *s, AsyncClient *c, int8_t error) {
	                DEBUG_MSG_FAUXMO("[FAUXMO] Error %s (%d) on client #%d\n", c->errorToString(error), error, i);
	            }, 0);

	            client->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
	                DEBUG_MSG_FAUXMO("[FAUXMO] Timeout on client #%d at %i\n", i, time);
	                c->close();
	            }, 0);

                    client->setRxTimeout(FAUXMO_RX_TIMEOUT);

	            DEBUG_MSG_FAUXMO("[FAUXMO] Client #%d connected\n", i);
	            return;

	        }

	    }

		DEBUG_MSG_FAUXMO("[FAUXMO] Rejecting - Too many connections\n");

	} else {
		DEBUG_MSG_FAUXMO("[FAUXMO] Rejecting - Disabled\n");
	}

    client->onDisconnect([](void *s, AsyncClient *c) {
        c->free();
        delete c;
    });
    client->close(true);

}

// -----------------------------------------------------------------------------
// Devices
// -----------------------------------------------------------------------------

fauxmoESP::~fauxmoESP() {
  	
	// Free the name for each device
	for (auto& device : _devices) {
		free(device.name);
  	}
  	
	// Delete devices  
	_devices.clear();

}

unsigned char fauxmoESP::addDevice(const char * device_name) {

    fauxmoesp_device_t device;
    unsigned int device_id = _devices.size();

    // init properties
    device.name = strdup(device_name);
	device.state = false;
	device.value = 0;

    // Attach
    _devices.push_back(device);

    DEBUG_MSG_FAUXMO("[FAUXMO] Device '%s' added as #%d\n", device_name, device_id);

    return device_id;

}

int fauxmoESP::getDeviceId(const char * device_name) {
    for (unsigned int id=0; id < _devices.size(); id++) {
        if (strcmp(_devices[id].name, device_name) == 0) {
            return id;
        }
    }
    return -1;
}

bool fauxmoESP::renameDevice(unsigned char id, const char * device_name) {
    if (id < _devices.size()) {
        free(_devices[id].name);
        _devices[id].name = strdup(device_name);
        DEBUG_MSG_FAUXMO("[FAUXMO] Device #%d renamed to '%s'\n", id, device_name);
        return true;
    }
    return false;
}

bool fauxmoESP::renameDevice(const char * old_device_name, const char * new_device_name) {
	int id = getDeviceId(old_device_name);
	if (id < 0) return false;
	return renameDevice(id, new_device_name);
}

bool fauxmoESP::removeDevice(unsigned char id) {
    if (id < _devices.size()) {
        free(_devices[id].name);
		_devices.erase(_devices.begin()+id);
        DEBUG_MSG_FAUXMO("[FAUXMO] Device #%d removed\n", id);
        return true;
    }
    return false;
}

bool fauxmoESP::removeDevice(const char * device_name) {
	int id = getDeviceId(device_name);
	if (id < 0) return false;
	return removeDevice(id);
}

char * fauxmoESP::getDeviceName(unsigned char id, char * device_name, size_t len) {
    if ((id < _devices.size()) && (device_name != NULL)) {
        strncpy(device_name, _devices[id].name, len);
    }
    return device_name;
}

bool fauxmoESP::setState(unsigned char id, bool state, unsigned char value) {
    if (id < _devices.size()) {
		_devices[id].state = state;
		_devices[id].value = value;
		return true;
	}
	return false;
}

bool fauxmoESP::setState(const char * device_name, bool state, unsigned char value) {
	int id = getDeviceId(device_name);
	if (id < 0) return false;
	_devices[id].state = state;
	_devices[id].value = value;
	return true;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool fauxmoESP::process(AsyncClient *client, bool isGet, String url, String body) {
	return _onTCPRequest(client, isGet, url, body);
}

void fauxmoESP::handle() {
    if (_enabled) _handleUDP();
}

void fauxmoESP::enable(bool enable) {

	if (enable == _enabled) return;
    _enabled = enable;
	if (_enabled) {
		DEBUG_MSG_FAUXMO("[FAUXMO] Enabled\n");
	} else {
		DEBUG_MSG_FAUXMO("[FAUXMO] Disabled\n");
	}

    if (_enabled) {

		// Start TCP server if internal
		if (_internal) {
			if (NULL == _server) {
				_server = new AsyncServer(_tcp_port);
				_server->onClient([this](void *s, AsyncClient* c) {
					_onTCPClient(c);
				}, 0);
			}
			_server->begin();
		}

		// UDP setup
		#ifdef ESP32
            _udp.beginMulticast(FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
        #else
            _udp.beginMulticast(WiFi.localIP(), FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
        #endif
        DEBUG_MSG_FAUXMO("[FAUXMO] UDP server started\n");

	}

}
