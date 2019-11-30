/*
Copyright (C) 2019 by Stephen Fisher 

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-->Simple Switch (Simple Switch) 
This is an example implementation of the fishyDevice library.  This implements an switch using the ESP8266-01 (ESP-01 - 8 pins) and a relay. These are 
often sold as a small kit allowing 5V in to the relay board for switching 120VAC with a socket for the 8pin ESP-01.   

FD-Device-Definitions.h - this header defines device settings for compiling (name, device-type, etc), it is common to all fishyDIYdevices. You can compile it as is and make the changes to the device settings via the web interface.
FD-Simple-Switch.ino - this is the main file with loop() and setup() functions, it is generally the same flow for all fishyDIYdevices. It also contains all the definitions for the custom (device type specific) functions.
FD-Simple-Switch.h - this header adds declarations for functions, adds device-type specific variables, settings, and global variables.

*/

#include "FD-Device-Definitions.h"      // device settings common to all devices (names, debug paramters, etc)
#include "fishyDevices.h"               // fishyDevice header for class
#include <Bounce2.h>                    // switch debouncing class (https://github.com/thomasfredericks/Bounce2)
#include "FD-Simple-Switch.h"           // global variables specific to Simple-Switchs

//create the object and pass in the const holding the FD-Simple-Switch's webpage control panel HTML
fishyDevice fD(WEBCTRLSTR);

//this is the base setup routine called first on power up or reboot
void setup()
{
    fD.FD_setup();
}

//this is the standard loop run for all fishy devices the customization occures in operateDevice
void loop()
{
    
    //-------------------------------------------------------------------------
    //Since we always want the manual switch to work with or without wifi we are 
    //putting this here instead of "operatedevice()"
    for(int i=0; i< NUM_SWITCHES;i++){
        debouncer[i].update(); // Update the Bounce instance
        if (debouncer[i].fell()) {
            //switch press detected
            //acknoweldge input
            fD.fastBlinks(1);
            //toggle relay
            if(DEBUG_MESSAGES){Serial.print("toggle " + String(i) + ": ");}
            toggleRelay(i);
        }
    }
    //-------------------------------------------------------------------------
    fD.checkResetOnLoop(); //reset device if flagged to by devic
    fD.checkWifiStatus();  //check/report on WiFi and support AP mode if not connected

    if (fD.myWifiConnect.connect && !fD.myWifiConnect.softAPmode)
    {
        //this is done first time we have credentials, after that just let autoreconnect handle temp losses
        fD.manageConnection();
    }
    else if (!fD.myWifiConnect.connect && !fD.myWifiConnect.softAPmode)
    {
        //connected and in STA mode - do normal loop stuff here
        fD.UDPprocessPacket();    //process any net (UDP) traffic
        fD.UDPkeepAliveAndCull(); //talk on net and drop dead notes from list
        fD.fauxmo.handle();       //handle voice commands via fauxmo
        fD.operateDevice();       //run device (state machine, commands, etc)
    }
    else
    {
    //in SoftAP mode - only do AP Wifi Config server stuff
    
    }
    fD.showHeapAndProcessSerialInput(); //if debugging allow heap display and take serial commands
}


//=============================================================================
// FUNCTION DEFINIITIONS-->

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
//Note: "normal" motion is open means motor is counting up (positive and away from zero) and that is defined as CW (100% will correspond
//to the highest motor count in the allowed range).  OpenIsCCW = true reverses that (0% relates to the lowest motor count).
String fishyDevice::getStatusString()
{
    String statusStr = "Switch:";

    for(int i=0; i< NUM_SWITCHES; i++){
        statusStr = statusStr + " " + String(i+1) + "-";
        if (EEPROMdeviceData.power_state[i] == 0)
        {
            statusStr = statusStr + "Off";
        }
        else
        {
            statusStr = statusStr + "On";
        }
    }
 
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getStatusString] " + String(statusStr));
    }
   
    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//generates a SHORT (max 3 characters long) status message to summarize this device's state on minimal displays. 
String fishyDevice::getShortStatString()
{
    String statusStr = "";
    for(int i=0; i< NUM_SWITCHES; i++){
        if (EEPROMdeviceData.power_state[i] == 0)
        {
            statusStr = statusStr + "0";
        }
        else
        {
            statusStr = statusStr + "1";
        }
    }
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getShortStatusString] " + String(statusStr));
    }

    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//encode compiled (settings) device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata for new devices
void fishyDevice::initializeDeviceCustomData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]start");
    }
    //default is off
    String builder = String("power_state={");
    for(int i = 0;i<NUM_SWITCHES-1;i++){
        builder +=  "0,";
    }
    builder += "0}";

    //get initial names from compiled settings
        switch(NUM_SWITCHES){
        case 1: //first name is "CUSTOM_DEVICE_NAME" and added automatically
        break;
        case 2:
        strncpy(EEPROMdeviceData.otherNames[0], DEVICE_2_NAME, MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[1], "", MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[2], "", MAXNAMELEN);
        break;
        case 3:
        strncpy(EEPROMdeviceData.otherNames[0], DEVICE_2_NAME, MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[1], DEVICE_3_NAME, MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[2], "", MAXNAMELEN);
        break;
        case 4:
        strncpy(EEPROMdeviceData.otherNames[0], DEVICE_2_NAME, MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[1], DEVICE_3_NAME, MAXNAMELEN);
        strncpy(EEPROMdeviceData.otherNames[2], DEVICE_4_NAME, MAXNAMELEN);
        break;

        if(DEBUG_MESSAGES){
            Serial.println("[initializeDeviceCustomData] Device Name = " + String(fD.myEEPROMdata.namestr));
            Serial.println("[initializeDeviceCustomData] Second device = " + String(EEPROMdeviceData.otherNames[0]));
            Serial.println("[initializeDeviceCustomData] Third device = " + String(EEPROMdeviceData.otherNames[1]));
            Serial.println("[initializeDeviceCustomData] Fourth device = " + String(EEPROMdeviceData.otherNames[2]));
        }

    }


    builder += "&names={" + String(fD.myEEPROMdata.namestr);
    for(int i = 1;i<NUM_SWITCHES;i++){
        builder += "," + String(EEPROMdeviceData.otherNames[i-1]);
    }
    //if(NUM_SWITCHES>1) {
    //    builder += String(EEPROMdeviceData.otherNames[NUM_SWITCHES-1]);
    //}
    builder +=  "}";
    strncpy(fD.myEEPROMdata.deviceCustomData, builder.c_str(), MAXCUSTOMDATALEN);
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//extract custom device data from char[MAXCUSTOMDATALEN] in fD.myEEPROMdata and put it into the device specific struct
void fishyDevice::extractDeviceCustomData()
{
    //example fD.myEEPROMdata.deviceCustomData = "openIsCCW=1&swapLimSW=0&motorPosAtCCWset=0&motorPosAtCWset=0&motorPos=2340&range=8889";
    char *strings[4];
    char *ptr = NULL;
    byte index = 0;

    if (DEBUG_MESSAGES)
    {
        Serial.println("[extractDeviceCustomData]start"); Serial.println("[extractDeviceCustomData]" + String(fD.myEEPROMdata.deviceCustomData));
    }
    ptr = strtok(fD.myEEPROMdata.deviceCustomData, "=&"); // takes a list of delimiters and points to the first token
    while (ptr != NULL)
    {
        strings[index] = ptr;
        index++;
        ptr = strtok(NULL, "=&"); // goto the next token
    }

    if (DEBUG_MESSAGES)
    {
        for (int n = 0; n < index; n++)
        {
            Serial.print(n);Serial.print(") ");Serial.println(strings[n]);
        }
    }

    //strings[1] is the switch state list
    for(int i = 0;i<NUM_SWITCHES-1;i++){
        //if(DEBUG_MESSAGES){ Serial.print(i);Serial.print("-->");Serial.print(i*2+1);Serial.println(": " + String(strings[1][i*2+1]));}
        EEPROMdeviceData.power_state[i] = (String(strings[1][i*2+1]) == "1") ? true : false;
    }
    
    //if(DEBUG_MESSAGES){Serial.print(NUM_SWITCHES-1);Serial.print("-->");Serial.print((NUM_SWITCHES-1)*2+1);Serial.println(": " + String(strings[1][(NUM_SWITCHES-1)*2+1])); }
    EEPROMdeviceData.power_state[NUM_SWITCHES-1] = (String(strings[1][(NUM_SWITCHES-1)*2+1]) == "1") ? true : false;
   
    //strings[3] is the other switch names list; device_name is already loaded by fishy device as the first switch name
    char* token = strtok(strings[3], ","); 
    token = strtok(NULL, ","); //skip first 
    int i = 0;
    while (token != NULL) { 
        strncpy(EEPROMdeviceData.otherNames[i], token, MAXNAMELEN);
        token = strtok(NULL, ",}"); 
        if (DEBUG_MESSAGES){
            Serial.println("[extractDeviceCustomData]" + String(EEPROMdeviceData.otherNames[i]));
        }
        i++;
    } 
    if (DEBUG_MESSAGES)
    {
        Serial.println("[extractDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::encodeDeviceCustomData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[encodeDeviceCustomData]start");
    }
    String builder = String("power_state={");
    for(int i = 0;i<NUM_SWITCHES-1;i++){
        builder += String(EEPROMdeviceData.power_state[i] ? "1" : "0") + ",";
    }
    builder += String(EEPROMdeviceData.power_state[NUM_SWITCHES-1] ? "1" : "0") + "}";
    builder += "&names={" + String(fD.myEEPROMdata.namestr);
    for(int i = 1;i<NUM_SWITCHES;i++){
        builder += "," + String(EEPROMdeviceData.otherNames[i-1]);
        if (DEBUG_MESSAGES){Serial.print("[encodeDeviceCustomData] otherNames[ " + String(i-1) + "]:");Serial.println(String(EEPROMdeviceData.otherNames[i-1]));}
    }
    builder +=  "}";
    if (DEBUG_MESSAGES)
    {
        Serial.print("[encodeDeviceCustomData]Encoded String: ");
        Serial.println(builder);
    }
    strncpy(fD.myEEPROMdata.deviceCustomData, builder.c_str(), MAXCUSTOMDATALEN);
    if (DEBUG_MESSAGES)
    {
        Serial.println("[encodeDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::showEEPROMdevicePersonalityData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[showEEPROMdevicePersonalityData]start");
        String builder = String("power_state={");
        for(int i = 0;i<NUM_SWITCHES-1;i++){
            builder = builder + String(EEPROMdeviceData.power_state[i] ? "1" : "0") + ",";
        }
        builder = builder + String(EEPROMdeviceData.power_state[NUM_SWITCHES-1] ? "1" : "0") + "}";
        Serial.println(builder);
        Serial.println("[showEEPROMdevicePersonalityData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//return true (no calibration needed)
bool fishyDevice::isCustomDeviceReady()
{    
    return true;
}

String fishyDevice::getDeviceSpecificJSON()
{
    String temp;
    temp = "{\"fishyDevices\":[";

    /* 
    put this Simple-Switch data in a string.
    Note - this is string will be parsed by scripts in the custom device webpage (webresource.h) - if adding data elements all these may need updating.  
    This function sends data as follows (keep this list updated):
    {ip,power_state,isMaster,name,port,note,swVer,devType,initStamp,timeOut,deviceTimedOut}
    */
    String builder = String("{");
    for(int i = 0;i<NUM_SWITCHES-1;i++){
        builder = builder + "\""+String(i+1)+"\":\"" + String(EEPROMdeviceData.power_state[i] ? "1" : "0") + "\",";
    }
    builder = builder + "\""+String(NUM_SWITCHES)+"\":\"" + String(EEPROMdeviceData.power_state[NUM_SWITCHES-1] ? "1" : "0") + "\"}";
    temp += "{\"ip\":\"" + WiFi.localIP().toString() +
            "\",\"NumSwitches\":\"" + String(NUM_SWITCHES) +
            "\",\"power_state\":" + builder +
            ",\"isMaster\":\"" + String(fD.myEEPROMdata.master ? "true" : "false") +
            "\",\"names\":";
    builder = String("{\"1\":\"") + String(fD.myEEPROMdata.namestr) + "\"";
    for(int i = 1;i<NUM_SWITCHES;i++){
        builder += ",\""+String(i+1)+"\":\"" + String(EEPROMdeviceData.otherNames[i-1]) + "\"";
    }
    builder +=  "}";
    
    temp += builder +     
            ",\"note\":\"" + String(fD.myEEPROMdata.note) + 
            "\",\"swVer\":\"" + String(fD.myEEPROMdata.swVer) +
            "\",\"devType\":\"" + String(fD.myEEPROMdata.typestr) + 
            "\",\"initStamp\":\"" + String(fD.myEEPROMdata.initstr) +
            "\",\"timeOut\":\"" + String(fD.myEEPROMdata.timeOut) +
            "\",\"deviceTimedOut\":\"" + String(fD.myEEPROMdata.deviceTimedOut ? "true" : "false") + "\"}";

    temp += "]}";
    return temp;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
void fishyDevice::operateDevice()
{
    


}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//NOTE: this runs even if server is in AP mode and not
//connected to WiFi - make sure this won't break
//THIS IS A FUNCTION FOR A Simple-Switch
void fishyDevice::deviceSetup()
{
    //pinSetup:
     // Attach the debouncer to a pin with INPUT mode for each switch
     // Use a debounce interval of 25 milliseconds for each switch
    for(int i=0;i<NUM_SWITCHES;i++){
        digitalWrite(relayPin[i], HIGH); //set off to start
        pinMode(pwrSwPin[i], INPUT_PULLUP);
        debouncer[i].attach(pwrSwPin[i], INPUT_PULLUP);
        debouncer[i].interval(25); 
        pinMode(relayPin[i], OUTPUT);
    }

    //At this point the state of the switch (if saved and then power lost momentarily or the device resarted) is known.
    //Set the relay to that state.
    for(int i=0;i<NUM_SWITCHES;i++){
        if(EEPROMdeviceData.power_state[i]){
            relayOn(i);
        }else{
            relayOff(i);
        }
    }   

    switch(NUM_SWITCHES){
        case 1: //first name is "CUSTOM_DEVICE_NAME" and added automatically
        break;
        case 2:
        addAnotherDevice(EEPROMdeviceData.otherNames[0]);   
        if(DEBUG_MESSAGES){
            Serial.println("[deviceSetup]Added Second device = " + String(EEPROMdeviceData.otherNames[0]));
            Serial.println("[deviceSetup]No Third or Fourth device");
        }
        break;
        case 3:
        addAnotherDevice(EEPROMdeviceData.otherNames[0]);
        addAnotherDevice(EEPROMdeviceData.otherNames[1]);   
        if(DEBUG_MESSAGES){
            Serial.println("[deviceSetup]Added Second device = " + String(EEPROMdeviceData.otherNames[0]));
            Serial.println("[deviceSetup]Added Third device = " + String(EEPROMdeviceData.otherNames[1]));
            Serial.println("[deviceSetup]No Fourth device");
        }
        break;
        case 4:
        addAnotherDevice(EEPROMdeviceData.otherNames[0]);
        addAnotherDevice(EEPROMdeviceData.otherNames[1]);
        addAnotherDevice(EEPROMdeviceData.otherNames[2]);   
        if(DEBUG_MESSAGES){
            Serial.println("[deviceSetup]Added Second device = " + String(EEPROMdeviceData.otherNames[0]));
            Serial.println("[deviceSetup]Added Third device = " + String(EEPROMdeviceData.otherNames[1]));
            Serial.println("[deviceSetup]Added Fourth device = " + String(EEPROMdeviceData.otherNames[2]));
        }
        break;
    }

}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Simple-Switch
//This function takes messages from some remote address (if from another node)
//that are of maximum length MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//RESERVED COMMANDS: {~udp~anyfishydev_there,~udp~fishydiymaster,~udp~poll_net,~udp~poll_response,reset_wifi,reset} -> these are handled within
//fishyDevice.cpp - if processed below that overides the default behavior.
// Return "true" if the command is processed; return "false" if not a device specific command to try and run the default commands next
//VALID SS DEVICE COMMANDS:
// (ON, OFF, config[;semi-colon-separated-parameters=values])
bool fishyDevice::executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
    String cmd = String(inputMsg);

    if (cmd.startsWith("on")) //(a "true" state recevied via Wifi)
    {
        if (DEBUG_MESSAGES)
        {
            Serial.print("[executeDeviceCommands] Commanded ON - sw #");
            Serial.println(cmd[2]);
        }
        //cmd should be on1,on2,on3, or on4 - take last cahr and subtract 1 to order device
        int device_id = cmd[2] - '0'- 1;
        switch (device_id)
        {
            case 0: 
            executeState(device_id, fD.myEEPROMdata.namestr, true, 255, 1); // (going on)
            break;
            default:
            executeState(device_id, EEPROMdeviceData.otherNames[device_id-1], true, 255, 1); // (going on)
            break;
        }
        //updateClients done in executeState

        return true;
    }
    else if (cmd.startsWith("off")) //(a "false" state recevied via Wifi)
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded OFF- sw #");
            Serial.println(cmd[3]);
        }
        //cmd should be off1,off2,off3, or off4 - take last cahr and subtract 1 to order device
        int device_id = cmd[3] - '0' -1;
        switch (device_id)
        {
            case 0: 
            executeState(device_id, fD.myEEPROMdata.namestr, false, 255, 1); // (going off)
            break;
            default:
            executeState(device_id, EEPROMdeviceData.otherNames[device_id-1], false, 255, 1); // (going off)
            break;
        }
        //updateClients done in executeState
       
        return true;
    }
    else if (cmd.startsWith("config"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded CONFIG");
        }
        UDPparseConfigResponse(inputMsg, remote);
        updateClients("Settings Updated.", true);
        return true;
    }
    else
    {
        return false;
    }
}

void fishyDevice::UDPparseConfigResponse(char inputMsg[MAXCMDSZ], IPAddress remote)
{
    char response[MAXCMDSZ] = "";
    strncpy(response, inputMsg + 7, MAXCMDSZ - 7); //strip off "CONFIG;"

    //parseString in this order: {isMaster, devName, note, timeOut}
    //example string = "isMaster=false;devName=New Device;note=;timeOut=60";
    char *strings[14]; //one string for each label and one string for each value
    char *ptr = NULL;
    byte index = 0;

    if (DEBUG_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] Got this: " + String(response));
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
    //isMaster
    fD.myEEPROMdata.master = strcmp(strings[1], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.print("[UDPparseConfigResponse] isMaster: ");
        Serial.println(fD.myEEPROMdata.master ? "true" : "false");
    }
    //devName (also sw1Name)
    strncpy(fD.myEEPROMdata.namestr, strings[3], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] devName (sw1Name): " + String(fD.myEEPROMdata.namestr));
    }
    //sw2Name
    strncpy(EEPROMdeviceData.otherNames[0],  strings[5], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] sw2Name: " + String(EEPROMdeviceData.otherNames[0]));
    }
    //sw3Name
    strncpy(EEPROMdeviceData.otherNames[1], strings[7], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] sw3Name: " + String(EEPROMdeviceData.otherNames[1]));
    }
    //sw4Name
    strncpy(EEPROMdeviceData.otherNames[2], strings[9], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] sw4Name: " + String(EEPROMdeviceData.otherNames[2]));
    }
    //note
    strncpy(fD.myEEPROMdata.note, strings[11], MAXNOTELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] note: " + String(fD.myEEPROMdata.note));
    }
    //timeOut
    fD.myEEPROMdata.timeOut = atoi(strings[13]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] timeOut: " + String(fD.myEEPROMdata.timeOut));
    }
    //inform the master of new settings
    UDPpollReply(masterIP);
    //save them to EEPROM
    storeDataToEEPROM();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
// execute state change
// This is used in 2 contexts: (1) an "On" or "Off" command received via web, UDP, etc and (2) a voice "On" or
// "Off" command being processed by fauxmo (e.g., through Alexa).
// -->In either context the switch will change to the order state; value is ignored here
void fishyDevice::executeState(unsigned char device_id, const char* device_name, bool state, unsigned char value, int context)
{
    //reset the timeout counter
    fD.deviceResponseTime = millis();
    fD.myEEPROMdata.deviceTimedOut = false;

    if (DEBUG_MESSAGES)
    {
        Serial.printf("[executeState] Device #%d (%s) commanded in context %d to state: %s value: %d\n", device_id, device_name, context, state ? "ON" : "OFF", value);
    }

    if (state) {
        relayOn(device_id);
        if (DEBUG_MESSAGES) Serial.println("Ran relayOn.");
        updateClients("Relay #" + String(device_id+1) + " ON", true); //true forces update and sends activity to MASTER
    }else{
        relayOff(device_id);
        if (DEBUG_MESSAGES) Serial.println("Ran relayOff.");
        updateClients("Relay #" + String(device_id+1) + " OFF", true);  //true forces update and sends activity to MASTER
    }
    //inform the master of new settings
    //UDPpollReply(masterIP);
}

//=============================================================================
//=============================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL
void toggleRelay(int num) {  
  if (EEPROMdeviceData.power_state[num]) {
    relayOff(num);
    if (DEBUG_MESSAGES) Serial.println("Ran Local Switch relayOff for switch #" + String(num+1));
  } else {
    relayOn(num);
    if (DEBUG_MESSAGES) Serial.println("Ran Local Switch relayOn for switch #" + String(num+1));
  }
}

void relayOff(int num) {
    //fD.fastBlinks(num+1);
    EEPROMdeviceData.power_state[num] = false; //False = off = RELAY_PIN to HIGH
    digitalWrite(relayPin[num], HIGH);
    //save to EEPROM
    fD.storeDataToEEPROM();
}

void relayOn(int num) {
    //fD.fastBlinks(num+1);
    EEPROMdeviceData.power_state[num] = true; //true = on = RELAY_PIN to LOW
    digitalWrite(relayPin[num], LOW);
    //save to EEPROM
    fD.storeDataToEEPROM();
}


String getPinMode(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  // I don't see an option for mega to return this, but whatever...
  if (NOT_A_PIN == port) return String("NOT_A_PIN");

  // Is there a bit we can check?
  if (0 == bit) return String("BIT == 0");

  // Is there only a single bit set?
  if (bit & bit - 1) return String("SINGLE BIT");

  volatile uint32_t *reg, *out;
  reg = portModeRegister(port);
  out = portOutputRegister(port);

  if (*reg & bit)
    return String("OUTPUT");
  else if (*out & bit)
    return String("INPUT_PULLUP");
  else
    return String("INPUT");
}