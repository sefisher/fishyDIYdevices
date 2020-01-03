/*
Copyright (C) 2019 by Stephen Fisher 

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-->Limit Switch Actuator (LSA) 
This is an example implementation of the fishyDevice library.  This implements an actuator driven by a stepper motor whose range of operation is lmited by two limit switches at either end of the actuator's travel. The switch defined as "open" will correspond to 100% (and on), the switch defined as "closed" will correspond to 0% (and off). Once calibrated (each switch position found by the device at least once) it can reliably accept commands to goto any potistion between 000 and 100 (%).  

FD-Limit-Switch-Actuator.ino - this is the main file with loop() and setup() functions, it is generally the same flow for all fishyDIYdevices. It also contains all the definitions for the custom (device type specific) functions.
FD-Limit-Switch-Actuator.h - this header adds declarations for functions, adds device-type specific variables, settings, and global variables.
FD-Device-Definitions.h - this header defines device settings for compiling (name, device-type, etc), it is common to all fishyDIYdevices. You can compile it as is and make the changes to the device settings via the web interface.

*/

#include <fishyDevices.h>               // fishyDevice header for class
#include <AccelStepper.h>               // a library for stepper motor control

#include "FD-Device-Definitions.h"      // device settings common to all devices (names, debug paramters, etc)
#include "FD-Limit-Switch-Actuator.h"   // global variables specific to Limit-SW-Actuators

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
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//generates a status message to be delivered for this devices state to the summary webpage for all the devices by the MASTER webserver
//Note: "normal" motion is open means motor is counting up (positive and away from zero) and that is defined as CW (100% will correspond
//to the highest motor count in the allowed range).  OpenIsCCW = true reverses that (0% relates to the lowest motor count).
String fishyDevice::getStatusString()
{
    String statusStr = "Current Position:";
    int pos;
    if (EEPROMdeviceData.range == 0)
    {
        statusStr = statusStr + "0% Open";
        if (DEBUG_MESSAGES)
        {
            Serial.println("[getStatusString] Range of 0 found.");
        }
    }
    else
    {
        pos = round(((float)EEPROMdeviceData.motorPos * 100.0) / (float)EEPROMdeviceData.range);
        if (EEPROMdeviceData.openIsCCW)
            pos = 100 - pos;
        statusStr = statusStr + String(pos) + "% Open";
    }
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getStatusString] Status:" + String(statusStr));
    }
    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//generates a SHORT (max 3 characters long) status message to summarize this device's state on minimal displays. 
String fishyDevice::getShortStatString()
{
    String statusStr = "";
    int pos;
    if (EEPROMdeviceData.range == 0)
    {
        statusStr = statusStr + "---";
        if (DEBUG_MESSAGES)
        {
            Serial.println("[getShortStatString] Range of 0 found.");
        }
    }
    else
    {
        pos = round(((float)EEPROMdeviceData.motorPos * 100.0) / (float)EEPROMdeviceData.range);
        if (EEPROMdeviceData.openIsCCW)
            pos = 100 - pos;
        statusStr = paddedInt(3,pos); //this function is built-in to fishyDevices
    }
    if (DEBUG_MESSAGES)
    {
        Serial.println("[getShortStatString] Status:" + String(statusStr));
    }
    return statusStr;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//encode compiled (settings) device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata for new devices
void fishyDevice::initializeDeviceCustomData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]start");
    }
    String builder = String("openIsCCW=") + String(OPEN_IS_CCW ? "1" : "0") + String("&swapLimSW=") + String(SWAP_LIM_SW ? "1" : "0") + String("&motorPosAtCCWset=0&motorPosAtCWset=0&motorPos=0&range=") + String(FULL_SWING);
    strncpy(fD.myEEPROMdata.deviceCustomData, builder.c_str(), MAXCUSTOMDATALEN);
    if (DEBUG_MESSAGES)
    {
        Serial.println("[initializeDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//extract custom device data from char[MAXCUSTOMDATALEN] in fD.myEEPROMdata and put it into the device specific struct
void fishyDevice::extractDeviceCustomData()
{
    //example fD.myEEPROMdata.deviceCustomData = "openIsCCW=1&swapLimSW=0&motorPosAtCCWset=0&motorPosAtCWset=0&motorPos=2340&range=8889";
    char *strings[12];
    char *ptr = NULL;
    byte index = 0;

    if (DEBUG_MESSAGES)
    {
        Serial.println("[extractDeviceCustomData]start");
        if (DEBUG_MESSAGES)
        {
            Serial.println("[extractDeviceCustomData]" + String(fD.myEEPROMdata.deviceCustomData));
        }
        // for(int ii=0;ii<MAXCUSTOMDATALEN;ii++){
        // 	Serial.print("(");Serial.print(ii);Serial.print(")");
        // 	Serial.print(fD.myEEPROMdata.deviceCustomData[ii]);Serial.print("-");
        // 	Serial.print(fD.myEEPROMdata.deviceCustomData[ii],DEC);
        // }
        // Serial.println("-------Done-------");
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
        //Serial.print(index);Serial.print("-");Serial.printf("Memory range: %x to %x\n", &strings[0], &strings[index-1]);
        for (int n = 0; n < index; n++)
        {
            Serial.print(n);
            Serial.print(") ");
            Serial.println(strings[n]);
        }
    }

    //names are even (0,2,4..), data is odd(1,3,5..)
    EEPROMdeviceData.openIsCCW = (String(strings[1]) == "1") ? true : false;
    EEPROMdeviceData.swapLimSW = (String(strings[3]) == "1") ? true : false;
    EEPROMdeviceData.motorPosAtCCWset = (String(strings[5]) == "1") ? true : false;
    EEPROMdeviceData.motorPosAtCWset = (String(strings[7]) == "1") ? true : false;
    EEPROMdeviceData.motorPos = atoi(strings[9]);
    EEPROMdeviceData.range = atoi(strings[11]);

    if (DEBUG_MESSAGES)
    {
        Serial.println("[extractDeviceCustomData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::encodeDeviceCustomData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[encodeDeviceCustomData]start");
    }
    String builder = "openIsCCW=" + String(EEPROMdeviceData.openIsCCW ? "1" : "0") + "&swapLimSW=" + String(EEPROMdeviceData.swapLimSW ? "1" : "0") + "&motorPosAtCCWset=" + String(EEPROMdeviceData.motorPosAtCCWset ? "1" : "0") + "&motorPosAtCWset=" + String(EEPROMdeviceData.motorPosAtCWset ? "1" : "0") + "&motorPos=" + String(EEPROMdeviceData.motorPos) + "&range=" + String(EEPROMdeviceData.range);
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
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//encode dynamic device data into the char[MAXCUSTOMDATALEN] for storage in fD.myEEPROMdata
void fishyDevice::showEEPROMdevicePersonalityData()
{
    if (DEBUG_MESSAGES)
    {
        Serial.println("[showEEPROMdevicePersonalityData]start");
        Serial.println("OpenIsCCW: " + String(EEPROMdeviceData.openIsCCW ? "True" : "False") + ", SwapLimSW: " + String(EEPROMdeviceData.swapLimSW ? "True" : "False"));
        Serial.println("Found motor data: {CCWset,CWset,Pos,range}: {" + String(EEPROMdeviceData.motorPosAtCCWset) + "," + String(EEPROMdeviceData.motorPosAtCWset) + "," + String(EEPROMdeviceData.motorPos) + "," + String(EEPROMdeviceData.range) + "}");
        Serial.println("[showEEPROMdevicePersonalityData]finish");
    }
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//return if the device is calibrated (the common function checks for deviceTimedOut)
bool fishyDevice::isCustomDeviceReady()
{
    //if both limit switch positions have been stored the device is calibrated
    if (EEPROMdeviceData.motorPosAtCCWset && EEPROMdeviceData.motorPosAtCWset)
    {
        return true;
    }
    else
    {
        return false;
    }
}

String fishyDevice::getDeviceSpecificJSON()
{
    String temp;
    temp = "{\"fishyDevices\":[";
    /* 
    put this Limit-SW-Actuator data in a string.
    Note - this is string will be parsed by scripts in the custom device webpage (webresource.h) - if adding data elements all these may need updating.  
    This function sends data as follows (keep this list updated):
    {ip,motorPosAtCWset,motorPosAtCCWset,isMaster,motorPos,name,openIsCCW,port,note,swVer,devType,initStamp,range,timeOut,deviceTimedOut,swapLimSW}
    */
    temp += "{\"ip\":\"" + WiFi.localIP().toString() +
            "\",\"motorPosAtCWset\":\"" + String(EEPROMdeviceData.motorPosAtCWset ? "true" : "false") +
            "\",\"motorPosAtCCWset\":\"" + String(EEPROMdeviceData.motorPosAtCCWset ? "true" : "false") +
            "\",\"isMaster\":\"" + String(fD.myEEPROMdata.master ? "true" : "false") + "\",\"motorPos\":\"" + String(int(stepper1.currentPosition())) +
            "\",\"deviceName\":\"" + String(fD.myEEPROMdata.namestr) + "\",\"openIsCCW\":\"" + String(EEPROMdeviceData.openIsCCW ? "true" : "false") +
            "\",\"note\":\"" + String(fD.myEEPROMdata.note) + "\",\"swVer\":\"" + String(fD.myEEPROMdata.swVer) +
            "\",\"devType\":\"" + String(fD.myEEPROMdata.typestr) + "\",\"initStamp\":\"" + String(fD.myEEPROMdata.initstr) +
            "\",\"range\":\"" + String(EEPROMdeviceData.range) + "\",\"timeOut\":\"" + String(fD.myEEPROMdata.timeOut) +
            "\",\"deviceTimedOut\":\"" + String(fD.myEEPROMdata.deviceTimedOut ? "true" : "false") +
            "\",\"swapLimSW\":\"" + String(EEPROMdeviceData.swapLimSW ? "true" : "false") + "\"}";

    temp += "]}";
    return temp;
}
//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
void fishyDevice::operateDevice()
{
    //operateLimitSwitchActuator:

    //get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
    int CCWlimSensorVal = 1;
    int CWlimSensorVal = 1;
    if (EEPROMdeviceData.swapLimSW == false)
    { //fix in case switches are miswired (easy to get confused on actuator contruction)
        CCWlimSensorVal = digitalRead(SWpinLimitCCW);
        CWlimSensorVal = digitalRead(SWpinLimitCW);
    }
    else
    {
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
        case fullCW:   //idle - do nothing
        case fullCCW:  //idle - do nothing
            break;
        case movingCCW: //moving CCW
            deviceTrueState = moveCCW();
            updateClients("Moving CCW");
            break;
        case movingCW: //moving CW
            deviceTrueState = moveCW();
            updateClients("Moving CW");
            break;
        case unknown:            //unknown state (bootup without stored HW limits)
            if (!CWlimSensorVal) //means at CW limit
            {
                EEPROMdeviceData.range = stepper1.currentPosition();
                EEPROMdeviceData.motorPosAtCWset = true;
                EEPROMdeviceData.motorPosAtCCWset = false;
                idleActuator(fullCW);
            }
            else if (!CCWlimSensorVal) //means at CCW limit
            {
                stepper1.setCurrentPosition(0);
                EEPROMdeviceData.motorPosAtCCWset = true;
                EEPROMdeviceData.range = FULL_SWING;
                EEPROMdeviceData.motorPosAtCWset = false;
                idleActuator(fullCCW);
            }
            else
            {
                EEPROMdeviceData.motorPosAtCWset = false;
                EEPROMdeviceData.motorPosAtCCWset = false;
            }
            break;
        }
        break;
    case movingCCWCal: //calibration in first stage	- moving toward motor position 0 and CCW limit switch
        EEPROMdeviceData.range = FULL_SWING;
        EEPROMdeviceData.motorPosAtCCWset = false;
        EEPROMdeviceData.motorPosAtCWset = false;
        updateClients("Calibrating - CCW");
        deviceTrueState = moveCCW();
        if (deviceTrueState == fullCCW)
        { //Done with stage 1 go onto CW stage
            deviceCalStage = movingCWCal;
            if (DEBUG_MESSAGES)
            {
                Serial.printf("[MAIN loop cal] Found CCW limit (0 by def'n). Set motorPos to 0. Going to calibration CW stage.\n");
            }
            executeState(true, 255, 1);
        }
        break;
    case movingCWCal: //calibration in second stage
        updateClients("Calibrating - CW");
        moveCW();
        if (deviceTrueState == fullCW)
        { //Done with stage 2 - done!
            deviceCalStage = doneCal;
            if (DEBUG_MESSAGES)
            {
                Serial.printf("[MAIN loop cal] Found CW limit (%d). Calibration complete.\n", EEPROMdeviceData.range);
            }
        }
        break;
    }
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//NOTE: this runs even if server is in AP mode and not
//connected to WiFi - make sure this won't break
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
void fishyDevice::deviceSetup()
{
    //pinSetup:
    //set switch pins to use internal pull_up resistor
    pinMode(SWpinLimitCW, INPUT_PULLUP);
    pinMode(SWpinLimitCCW, INPUT_PULLUP);

    //motorSetup:
    //stepper motor setup
    stepper1.setMaxSpeed(MAX_SPEED);
    stepper1.setAcceleration(ACCELERATION);
    stepper1.setSpeed(0);
    stepper1.setCurrentPosition(EEPROMdeviceData.motorPos);
    if ((EEPROMdeviceData.motorPosAtCCWset + EEPROMdeviceData.motorPosAtCWset) == 2)
    {
        //if prior HW limits set state to man_idle to prevent unknown state effects
        deviceTrueState = man_idle;
    }
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A Limit-SW-Actuator
//This function takes messages from some remote address (if from another node)
//that are of maximum length MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//RESERVED COMMANDS: {~udp~anyfishydev_there,~udp~fishydiymaster,~udp~poll_net,~udp~poll_response,reset_wifi,reset} -> these are handled within
//fishyDevice.cpp - if processed below that overides the default behavior.
// Return "true" if the command is processed; return "false" if not a device specific command to try and run the default commands next
//VALID LSA DEVICE COMMANDS:
// (open,close,stop,gotoXXX,calibrate,config[;semi-colon-separated-parameters=values])
bool fishyDevice::executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
    String cmd = String(inputMsg);

    if (cmd.startsWith("open")) //Open is "Full ON" (a "true" state recevied via Wifi)
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded OPEN");
        }
        updateClients("Open Received", true);

        if (EEPROMdeviceData.openIsCCW)
        {
            executeState(true, 000, 1); // (going full CCW to stop for OpenisCCW=true)
        }
        else
        {
            executeState(true, 255, 1); // (going full CW to stop for OpenisCCW=false)
        }
        return true;
    }
    else if (cmd.startsWith("close")) ////Close is "OFF" (a "false" state recevied via Wifi)
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded CLOSE");
        }
        updateClients("Close Received", true);
        if (EEPROMdeviceData.openIsCCW)
        {
            executeState(false, 255, 1); // (going full CW to stop for OpenisCCW=true)
        }
        else
        {
            executeState(false, 000, 1); // (going full CCW to stop for OpenisCCW=false)
        }
        return true;
    }
    else if (cmd.startsWith("stop"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded STOP");
        }
        updateClients("Stop Received", true);
        executeStop();
        return true;
    }
    else if (cmd.startsWith("goto"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded GOTO (" + cmd + ")");
        }
        updateClients("Goto (" + cmd + ") Received", true);
        executeGoto(cmd);
        return true;
    }
    else if (cmd.startsWith("calibrate"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded CALIBRATE");
        }

        deviceCalStage = movingCCWCal;
        if (DEBUG_MESSAGES)
        {
            Serial.printf("[executeDeviceCommands] Going to calibration full CCW stage.\n");
        }
        updateClients("Calibrating", true);
        executeState(true, 0, 1); //goto full CCW first
        return true;
    }
    else if (cmd.startsWith("config"))
    {
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeDeviceCommands] Commanded CONFIG");
        }
        UDPparseConfigResponse(inputMsg, remote); //want the original case for this
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

    //parseString in this order: {openIsCCW, isMaster, devName, note, swapLimSW, timeOut}
    //example string = "openIsCCW=false;isMaster=false;devName=New Device;note=;swapLimSW=false;timeOut=60";
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
    //openIsCCW
    EEPROMdeviceData.openIsCCW = strcmp(strings[1], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.print("[UDPparseConfigResponse] isMaster: ");
        Serial.println(EEPROMdeviceData.openIsCCW ? "true" : "false");
    }
    //isMaster
    fD.myEEPROMdata.master = strcmp(strings[3], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.print("[UDPparseConfigResponse] isMaster: ");
        Serial.println(fD.myEEPROMdata.master ? "true" : "false");
    }
    //devName
    strncpy(fD.myEEPROMdata.namestr, strings[5], MAXNAMELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[UDPparseConfigResponse] devName: " + String(fD.myEEPROMdata.namestr));
    }
    //note
    strncpy(fD.myEEPROMdata.note, strings[7], MAXNOTELEN);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] note: " + String(fD.myEEPROMdata.note));
    }
    //swapLimSW
    EEPROMdeviceData.swapLimSW = strcmp(strings[9], "false") == 0 ? false : true;
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.print("[UDPparseConfigResponse] swapLimSW: ");
        Serial.println(EEPROMdeviceData.swapLimSW ? "true" : "false");
    }
    //timeOut
    fD.myEEPROMdata.timeOut = atoi(strings[11]);
    if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
    {
        Serial.println("[RGBUDPparseConfigResponse] timeOut: " + String(fD.myEEPROMdata.timeOut));
    }
    //inform the master of new settings
    UDPpollReply(masterIP);
    //save them to EEPROM
    storeDataToEEPROM();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (FOR ALL FAUXMO ENABLED DEVICES)
// execute state change
// This is used in 2 contexts: (1) an "Open" or "Close" command received via web, UDP, etc and (2) a voice "On" or
// "Off" command being processed by fauxmo (e.g., through Alexa).
// -->In context (1) the actuator will move to the indicated limit switch (e.g., Open with OpenIsCCW=false will move to full CW)
// -->In context (2); First note that fishyDevice functions (and therefore faxumo/Alexa) cannot know the direction
// 		corresponding to On/Open Off/Closed or how to translate the value 000 to 255 given into a motor position since
// 		they don't have "OpenIsCCW".  So that has to be done within this function.
// 		--ON=OPENXXX equates to "goto back to last non-closed position" (value provided by fauxmo)
// 		--OFF=CLOSE equates to "goto closed limit switch position"
// the context is determined outside this function so for context (1) value is either 000 or 255, in context (2) it is 000 or XXX or 255
// for either 000 or 255 - the motor will run fast to limit switch. For XXX it uses the executeGoto function which ramps speed up and down.
void fishyDevice::executeState(bool state, unsigned char value, int context)
{
    unsigned char correctedValue = value;
    //reset the motor timeout counter
    fD.deviceResponseTime = millis();
    fD.myEEPROMdata.deviceTimedOut = false;

    //ensure motor output is enabled
    stepper1.enableOutputs();

    if (DEBUG_MESSAGES)
    {
        Serial.printf("[executeState] Commanded in context %d to state: %s value: %d\n", context, state ? "ON=OPEN" : "OFF=CLOSED", value);
    }

    if ((context == 2))
    //fauxmo is ignorant of open or closed direction - need to correct for that
    //recall fauxmo voice commands are either "on" (go back to last open position given by value)",
    // or "set to XXX" (turn on and goto new position given by value), or "off" (turn off and goto 000 or 255 based on openIsCCW)
    {
        if (EEPROMdeviceData.openIsCCW)
        {
            if (state == false) //turn off (close) and open is CCW
            {
                correctedValue = 255;
            }
            else //turn on and open is CCW
            {
                correctedValue = 255 - value;
            }
        }
        else //open is CW
        {
            if (state == false) //turn off
            {
                correctedValue = 0;
            }
        }
    } //at this point context 2 (voice through fauxmo) is corrected so we can treat is the same as other cases

    //for context 1 (have device knowledge) - the outside functions have figured all that out so just goto where
    //told to (ignore state), do it fast for 255 or 000, use ramps for anything else
    if (correctedValue == 255)
    {
        targetPos = -1;
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeState] Moving CW to limit SW...");
        }
        deviceTrueState = moveCW();
    }
    else if (correctedValue == 0)
    {
        targetPos = -1;
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeState] Moving CCW to limit SW...");
        }
        deviceTrueState = moveCCW();
    }
    else
    {
        //this won't get executed in context 2 since in that context this function is used to only go to one of the limit switches
        //range is 0 to 255 from fauxmo; we need to convert that to 0 to 100 percent and then move the actuator
        if (DEBUG_MESSAGES)
        {
            Serial.println("[executeState] Going to fauxmo commanded position...");
        }
        deviceTrueState = openPercentActuator(round(correctedValue * 100.0 / 255.0));
    }
}
//=============================================================================
//=============================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL

//CUSTOM DEVICE FUNCTION - INTERNAL
//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
    Serial.printf("got %s", cmd.c_str());
    //reset the motor timeout counter
    fD.deviceResponseTime = millis();
    fD.myEEPROMdata.deviceTimedOut = false;
    //ensure motor output is enabled
    stepper1.enableOutputs();
    int newPercentOpen = whichWayGoto(cmd.substring(4).toInt()); //STRIP OFF GOTO and correct for openisCCCW
    if (newPercentOpen == 100)
    {
        targetPos = -1;
        deviceTrueState = moveCCW();
    }
    else if (newPercentOpen == 0)
    {
        targetPos = -1;
        deviceTrueState = moveCW();
    }
    else
    { //targetting mid position
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
    fD.deviceResponseTime = millis();
    fD.myEEPROMdata.deviceTimedOut = false;
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
    int CCWlimSensorVal = 1;
    int CWlimSensorVal = 1;
    if (EEPROMdeviceData.swapLimSW == false)
    { //fix in case switches are miswired (easy to get confused on linear actuator)
        CCWlimSensorVal = digitalRead(SWpinLimitCCW);
        CWlimSensorVal = digitalRead(SWpinLimitCW);
    }
    else
    {
        CCWlimSensorVal = digitalRead(SWpinLimitCW);
        CWlimSensorVal = digitalRead(SWpinLimitCCW);
    }
    trueState newState;
    if (CCWlimSensorVal == 0)
    {
        //reached limit - disable everything and reset motor to idle
        // -set stepper pos = 0;  (by definition fullCCW is 0) and
        // -update flag indicating the HW lim sw was found
        stepper1.setCurrentPosition(0);
        EEPROMdeviceData.motorPosAtCCWset = true;
        newState = idleActuator(fullCCW); //make actuator idle
        if (DEBUG_MESSAGES)
        {
            Serial.printf("[moveCCW] Reached CCW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
        }
    }
    else
    { //keep going if still have distance to travel
        newState = movingCCW;

        if (targetPos > -1)
        {
            //if true then moving to a given position
            stepper1.run();
            //see if done or if timedout
            if ((stepper1.currentPosition() == targetPos))
            {
                newState = idleActuator(man_idle);
            }
            if ((millis() - fD.deviceResponseTime) > fD.myEEPROMdata.timeOut * 1000)
            {
                fD.myEEPROMdata.deviceTimedOut = true;
                newState = idleActuator(man_idle);
            }
        }
        else
        {
            if (stepper1.speed() != -MAX_SPEED)
            {
                stepper1.setSpeed(-MAX_SPEED);
            }
            stepper1.runSpeed();
            //see if timedout
            if (((millis() - fD.deviceResponseTime) > fD.myEEPROMdata.timeOut * 1000))
            {
                fD.myEEPROMdata.deviceTimedOut = true;
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
    int CCWlimSensorVal = 1;
    int CWlimSensorVal = 1;
    if (EEPROMdeviceData.swapLimSW == false)
    { //fix in case switches are miswired (easy to get confused on linear actuator)
        CCWlimSensorVal = digitalRead(SWpinLimitCCW);
        CWlimSensorVal = digitalRead(SWpinLimitCW);
    }
    else
    {
        CCWlimSensorVal = digitalRead(SWpinLimitCW);
        CWlimSensorVal = digitalRead(SWpinLimitCCW);
    }

    trueState newState;
    if (CWlimSensorVal == 0)
    {
        //reached limit - disable everything and reset motor to idle
        // -set range to current stepper pos and
        // -update flag indicating the HW lim sw was found
        EEPROMdeviceData.range = stepper1.currentPosition();
        EEPROMdeviceData.motorPosAtCWset = true;

        newState = idleActuator(fullCW); //make actuator idle
        if (DEBUG_MESSAGES)
        {

            Serial.printf("[moveCW] Reached CW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
        }
    }
    else
    { //keep going if still have distance to travel
        newState = movingCW;

        if (targetPos > -1)
        {
            //if true then moving to a given position using acceleration
            stepper1.run();
            //see if done or if timedout
            if ((stepper1.currentPosition() == targetPos))
            {
                newState = idleActuator(man_idle);
            }
            if ((millis() - fD.deviceResponseTime) > fD.myEEPROMdata.timeOut * 1000)
            {
                fD.myEEPROMdata.deviceTimedOut = true;
                newState = idleActuator(man_idle);
            }
        }
        else
        {
            if (stepper1.speed() != MAX_SPEED)
            {
                stepper1.setSpeed(MAX_SPEED);
            }
            stepper1.runSpeed();
            //see if timedout
            if (((millis() - fD.deviceResponseTime) > fD.myEEPROMdata.timeOut * 1000))
            {
                fD.myEEPROMdata.deviceTimedOut = true;
                newState = idleActuator(man_idle);
            }
        }
    }
    return newState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as
//open by openIsCCW
int whichWayGoto(int in)
{
    int ret = in;
    if (EEPROMdeviceData.openIsCCW == true)
    {
        ret = 100 - ret;
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
    fD.storeDataToEEPROM();
    fD.UDPpollReply(fD.masterIP); //tell the Master Node the new info
    fD.updateClients("Stopped.", true);
    return idleState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this detemines the target motor position from a 0 to 100 percent input
//and then which way to goto to get to a commanded position
//sets targetPos and commands open or close
//motor CCW limit = 0, motor CW limit = range, if no target set targetPos = -1
trueState openPercentActuator(int percent)
{
    trueState newState;
    int newMotorPos = (int)(EEPROMdeviceData.range * percent / 100);
    targetPos = newMotorPos;

    //get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
    int CCWlimSensorVal = 1;
    int CWlimSensorVal = 1;
    if (EEPROMdeviceData.swapLimSW == false)
    { //fix in case switches are miswired (easy to get confused on linear actuator)
        CCWlimSensorVal = digitalRead(SWpinLimitCCW);
        CWlimSensorVal = digitalRead(SWpinLimitCW);
    }
    else
    {
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
            newState = movingCCW;
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
            newState = idleActuator(fullCCW);
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
            newState = movingCW;
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
            newState = idleActuator(fullCW);
            if (DEBUG_MESSAGES)
            {
                Serial.printf("[openPercentActuator] Reached CW limit. Pos: %d\n", stepper1.currentPosition());
            }
        }
        return newState;
    }
}
//=============================================================================
