//example of a Limit-SW-Actuator device using the fishyDevice library

#include <fishyDevices.h>          // fishyDevice header for class
#include <AccelStepper.h>          // a library for stepper motor control

#include "FD-Device-Definitions.h" // device settings common to all devices (names, debug paramters, etc)
#include "FD-LSA-Globals.h"        // global variables specific to Limit-SW-Actuators
#include "FD-LSA-Web-Resources.h"  // webresources specific to Limit-SW-Actuators


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
