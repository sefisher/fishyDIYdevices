/*
Copyright (C) 2019 by Stephen Fisher 

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

THIS IS A TEMPLATE FOR YOUR OWN CUSTOM DEVICE.  SEE OTHER COMPLETED EXAMPLE DEVICE FILES FOR...WELL...EXAMPLES.

*/

#include <fishyDevices.h>               // fishyDevice header for class

//TODO 6 - add any other headers needed for your FD-[device name].ino here.

#include "FD-Device-Definitions.h"      // device settings common to all devices (names, debug paramters, etc)

//TODO 7 - update this name 
#include "FD-TEMPLATE.h"                // global variables specific to [TEMPLATE]s

//create the object and pass in the const holding the FD-Limit-Switch-Actuator's webpage control panel HTML
fishyDevice fD(WEBCTRLSTR);

//this is the base setup routine called first on power up or reboot
void setup()
{
    fD.FD_setup();
}

//this is the standard loop run for all fishy devices the customization occures in operateDevice
void loop()
{

    fD.checkResetOnLoop(); //reset device if flagged to by device
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
        fD.slowBlinks(1); //in SoftAP mode - only do AP Wifi Config server stuff
    }
    fD.showHeapAndProcessSerialInput(); //if debugging allow heap display and take serial commands
}


//=============================================================================
// FUNCTION DEFINIITIONS-->

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
//Note: "normal" motion is open means motor is counting up (positive and away from zero) and that is defined as CW (100% will correspond
//to the highest motor count in the allowed range).  OpenIsCCW = true reverses that (0% relates to the lowest motor count).
String fishyDevice::getStatusString()
{
    //TODO 8 - create function to return a string that reports the status of the device for display in a control panel
    
    String statusStr = "";

    }
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getStatusString] Status:" + String(statusStr));
    }
    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//generates a SHORT (max 3 characters long) status message to summarize this device's state on minimal displays. 
String fishyDevice::getShortStatString()
{
    //TODO 9 - create function to return a 3-character string that summarizes the status of the device for display in a control panel
    String statusStr = "";
    
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getShortStatString] Status:" + String(statusStr));
    }
    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//encode compiled (settings) device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata for new devices
void fishyDevice::initializeDeviceCustomData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]start");
    }
    
    //TODO 10 - create a string from the custom device settings and copy it into the myEEPROMdata.deviceCustomData 
    
    
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//extract custom device data from char[MAXCUSTOMDATALEN] in fD.myEEPROMdata and put it into the device specific struct
void fishyDevice::extractDeviceCustomData()
{

    //TODO 11 - build extracting function to update device settings based on data in string

    if (DEBUG_MESSAGES)
    {
        Serial.println("[extractDeviceCustomData]finish");
    }

 
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::encodeDeviceCustomData()
{
    //TODO 12 - build encdoding function to make string based on device settings  - see example below

    if (DEBUG_MESSAGES)
    {
        Serial.println("[encodeDeviceCustomData]start");
    }
    /*
    String builder = "openIsCCW=" + String(EEPROMdeviceData.openIsCCW ? "1" : "0") + "&swapLimSW=" + String(EEPROMdeviceData.swapLimSW ? "1" : "0") + "&motorPosAtCCWset=" + String(EEPROMdeviceData.motorPosAtCCWset ? "1" : "0") + "&motorPosAtCWset=" + String(EEPROMdeviceData.motorPosAtCWset ? "1" : "0") + "&motorPos=" + String(EEPROMdeviceData.motorPos) + "&range=" + String(EEPROMdeviceData.range);
    if (DEBUG_MESSAGES)
    {
        Serial.print("[encodeDeviceCustomData]Encoded String: ");
        Serial.println(builder);
    }
    strncpy(fD.myEEPROMdata.deviceCustomData, builder.c_str(), MAXCUSTOMDATALEN);
    */
    if (DEBUG_MESSAGES)
    {
        Serial.println("[encodeDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::showEEPROMdevicePersonalityData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[showEEPROMdevicePersonalityData]start");

        //TODO 13 - build decoding/serial.prinln function to display data 

        Serial.println("[showEEPROMdevicePersonalityData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//return if the device is calibrated (the common function checks for deviceTimedOut)
bool fishyDevice::isCustomDeviceReady()
{
    //TODO 14 - if needed determine if device is ready based on device paramaters and then return true/false.  Other just 
    return true;
}

String fishyDevice::getDeviceSpecificJSON()
{
    String temp;
    temp = "{\"fishyDevices\":[";
   
    //TODO 15 - put this [TEMPLATE] data in a string.
    //Note - this is string will be parsed by scripts in the custom device webpage - if adding data elements all these may need updating.  
    
    temp += "]}";
    return temp;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
void fishyDevice::operateDevice()
{
    //TODO 16 - Do any operations for your device that are required for every main operating loop cycle here (state engine, movement, updates, etc) 
    // use updateClients([string]); to send updates to any connected web controller about operations when they happen
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//NOTE: this runs even if server is in AP mode and not
//connected to WiFi - make sure this won't break in that mode
//THIS IS A FUNCTION FOR A [TEMPLATE]
void fishyDevice::deviceSetup()
{
    //TODO 17 - put any custom code here to setup the device the fist time it is powered on
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//This function takes messages from some remote address (if from another node)
//that are of maximum length MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//RESERVED COMMANDS: {~udp~anyfishydev_there,~udp~fishydiymaster,~udp~poll_net,~udp~poll_response,reset_wifi,reset} -> these are handled within
//fishyDevice.cpp - if processed below that overides the default behavior.
// Return "true" if the command is processed; return "false" if not a device specific command to try and run the default commands next
bool fishyDevice::executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
    //TODO 17 - customize below to check for your unique device commands in the inputMsg, if found do what is indicated and return true.
    // otherwise return false.  Below is default (accepts a configuration update for things and parses them).
   
    String cmd = String(inputMsg);

    if (cmd.startsWith("config"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded CONFIG");
        }
        UDPparseConfigResponse(inputMsg, remote); //want the original case for this
        updateClients("Settings Updated.", true);
        return true;
    }
   
   return false;

}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A [TEMPLATE]
//This function parses a string sent from a controlling webpage to update the devices configuration
//take that string (your design), decode it, and update the configuration
void fishyDevice::UDPparseConfigResponse(char inputMsg[MAXCMDSZ], IPAddress remote)
{
    //TODO 18 - customize code below to add any unique parsed strings and update settings; below is default
    
    char response[MAXCMDSZ] = "";
    strncpy(response, inputMsg + 7, MAXCMDSZ - 7); //strip off "CONFIG;"

    //parseString in this order: {isMaster, devName, note, timeOut}
    //example string = "isMaster=false;devName=New Device;note=;timeOut=60";
    char *strings[8]; //one string for each label and one string for each value
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
    //devName
    strncpy(fD.myEEPROMdata.namestr, strings[3], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] devName: " + String(fD.myEEPROMdata.namestr));
    }
    //note
    strncpy(fD.myEEPROMdata.note, strings[5], MAXNOTELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] note: " + String(fD.myEEPROMdata.note));
    }
    //timeOut
    fD.myEEPROMdata.timeOut = atoi(strings[7]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] timeOut: " + String(fD.myEEPROMdata.timeOut));
    }
    //inform the master of new settings
    UDPpollReply(masterIP);
    //save them to EEPROM
    storeDataToEEPROM();

    //inform the master of new settings
    UDPpollReply(masterIP);
    //save them to EEPROM
    storeDataToEEPROM();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
// execute state change
// This is used in 2 contexts: (1) an "On" or "Off" command received via web, UDP, etc and (2) a voice "On" or
// "Off" command being processed by fauxmo (e.g., through Alexa). You need to determine the reponse to those commands
// and take the right action in each context. Value provides a number corresponding to the 0 to 100 from a voice command.
void fishyDevice::executeState(bool state, unsigned char value, int context)
{
    //TODO 19 - make the code to execute the new device state for each context 
}
//=============================================================================
//=============================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL

//TODO 20 - put any code and helper functions here that are unique to your device

//=============================================================================
