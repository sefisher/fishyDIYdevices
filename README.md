# fishyDIYdevices
A library to simplify creating, operating, and using DIY Internet of Things devices using NodeMCU (ESP8266) devices. Enables voice control using Amazon Echo (Alexa).

## Who and what is this for?
### Who 
This is for anyone wanting a quick way to take cheap Internet of Things (IoT) chips and put together some DIY home automation devices with Alexa voice control.

### What
This library sets up interoperating home automation devices deployed on a local wireless network (no Internet dependencies). It is designed to work on ESP8266 (these are open source chip designs also known as Esperrif ESP8266, NodeMCU, and packaged under a variety of manufacture names). This library is set up to use the Arduino programming language. It makes extensive use of Hristo Gochkov's ESP Async toolset and Xose Pérez's fauxoESP library.

In addition to providing a simple framework for control and device-device, device-user communications this library takes care of the following basic functions:
 1) Setting/changing/storing WiFi crendtials without a wired connection.
 2) Providing over the air software updating (intial software load on any device is via usb at your computer).
 3) Creates an Alexa (Amazon Echo) voice control interface for controlling the devices.
 4) Displaying a master list with all local device's status and controls. 
 5) Mapview (**under development**) to see and control your devices overlaid on a layout/map of your home. (Logging, monitoring, and controlling in this way uses a separate monitoring device like a raspberry pi).
 
## Dependencies

Besides the libraries already included with the Arduino Core for ESP8266, these libraries are also required:

FauxmoEsp by [Xose Perez](http://tinkerman.cat/):
* [fauxmoesp](https://bitbucket.org/xoseperez/fauxmoesp) - tested and developed with version 3.0.0

ESPAsyncTCP and ESPAsyncWebServer by [me-no-dev](https://github.com/me-no-dev) (Hristo Gochkov):
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) library
* [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) library.

### Arduino IDE

If you are using the Arduino IDE you will need to install the fishyDevice and other required libraries from sources (click on ZIP on the 4 files below). If you are just getting started it is easiest to download a library as a ZIP file and install it using the option under "Sketch > Include Library > Add .ZIP Library...".

Here are the direct ZIP file locations here for convenience:

|Device|Library|Repository|ZIP|
|-|-|-|-|
|ESP8266|**fishyDIYdevices** by Stephen Fisher ESP8266|[GIT](https://github.com/sefisher/fishyDIYdevices)|[ZIP](https://github.com/sefisher/fishyDIYdevices/archive/Main.zip)|
|ESP8266|**ESPAsyncTCP** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncTCP)|[ZIP](https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip)|
|ESP8266|**ESPAsyncWebServer** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncWebServer)|[ZIP](https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip)|
|ESP8266|**FauxmoEsp** by Xose Perez ESP8266|[GIT](https://bitbucket.org/xoseperez/fauxmoesp)|[ZIP](https://bitbucket.org/xoseperez/fauxmoesp/get/f60c46d80f9b.zip)|

## Examples
* Detailed build instructions for the example devices (including files for 3D printed parts, hardware lists, etc) will be available at [fishyDIY.com](http://fishydiy.com/). These full project examples are a work in progress. See the README.md in the examples folders for simple instructions for each example there.
* The examples provided can generally be compiled and uploaded as is if you use the same electronic hardware. There are template html files in each example directory provided in case you want to customize the interfaces or make your own new device type. 
The following built-in device types are provided with hardware examples:
    - Limit-SW-Actuator (limit switches allow moving between full open and shut, or positioning anywhere from 0-100% of that range). This is a pretty flexible device that can be set up mechanical for a linear actuator (moving things back and forth) or rotating.  Using 2 limit switches makes positioning reliable since it can regain track on position if the motor ever "slips" (tries to move a step but doesn't have enough force). Using ESP8266, 28BYJ-48 stepper, ULN2003 driver, your 3D printed mechanical device, and a few switches you can have an actuator working for less than $8.
* Examples for the following types are under development:
    - RGBLED controller (colors and dimming control for a low power RGB LED strip)
    - Light switch controls (for replacing normal toggle light switches).
    - Infrared motion sensors (passive sensors to detect motion and trigger events).
    - Temperature sensors (provides room temperatures).
    - Single-SW-Actuator (does the same thing as the Limit-SW-Actuator, but on has a single limit switch to reset motion tracking from a  known point (limit SW) - the other stop point is set by software (counts motot rotations). It is slightly less reliable than a full limit switch actuator with two switches - but can be useful for things like roller blinds where a second siwtch is hard to implement. 

## Security
From a security standpoint - fishyDevices are intended to operate on a home WiFi network with device-device data transfer protected by your properly secured router (using the router's security to have device to device comms encrypted at the link layer). Each device can be controlled using a self-served control panel using any webbrowser only on your local network. Remote control (from outside your local network) is only enabled through your Alexa devices and Amazon.

## Usage
In case you are not a do-it-by-example person, or if you want to understand or modify the library.  The following two sections are for you.

### What are the three files in each example for?
Coming...

### Explain what I have to do in my main Arduino .ino file.
To make you own device the following basic "scaffolding" is needed by the fishyDevice library.  The rest (defining what your device actually does) is up to you.  

#### Required header files and the fD variable.
In this section you also include any other headers your device needs (like AccelStepper.h to control motors in the Limit Switch Actuator example). 
```
#include <fishyDevices.h>               // fishyDevice header for class
#include "FD-Device-Definitions.h"      // device settings common to all devices (names, debug paramters, etc)
#include "FD-[YOUR-DEVICE-TYPE-NAME].h"   // global variables specific to your device type

//create the object and pass in the const holding the your device's webpage control panel HTML (the default is "WEBCTRLSTR" with that character array is defined in the device's .h file).
fishyDevice fD(WEBCTRLSTR);
```
#### Required setup() function call.
```
void setup()
{
    fD.FD_setup();
}
```

#### Basic loop() flow.
```
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
```

#### Functions you MUST define (but are declared in the library)
The library needs these functions to work, you define them to control what they do in your specific device. But they are already declared (named with the input and output setup in the fishyDevice.h file). In the examples the comments before them say "//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME FUNCTION CALLED BY ALL DEVICES)". Each of the following functions needs to be coded in your .ino file. If you don't need that specific function to do anything in your device, you can just define it as a function shell with no internal code. (Note: MAXCMDSZ is a constant set to define the maximum command size, set 300 characters in the library. Commands are just text passed between devices and web control interfaces to make things happen.)

* **void operateDevice()** - This is run very loop cycle to make the device work based on user input
* **void deviceSetup()** - This is run at initialization (from setup()) to startup the device
* **bool executeDeviceCommands(char inputMsg[MAXCMDSZ], IPAddress remote)** - This is run by executecommands first to allow device specific commands to be processed (can override built-in commands processes as well)
* **void executeState(bool state, unsigned char value, int context)** - This executes voice command state changes (immplemented by Fauxmo - takes on/off or a numerical setting from 0-100 (e.g., "Alexa set My Device to 60"))
* **void UDPparseConfigResponse(char inputMsg[MAXCMDSZ], IPAddress remote)** - parse configuration string data to update all the stored parameters
* **String getStatusString()** - This returns a string with the device's status for display
* **String getShortStatString()** - This returns a SHORT string (3 char max) summarizing the device's status for minimalist displays (used for viewing/controlling devices overlaid on floor plans)
* **bool isCustomDeviceReady()** - This reports if the device is ready, if not sets Error flag. For example if a device needs calibrating one time to know its starting position, this can flag an ERROR to the user until that is done. 
* **String getDeviceSpecificJSON()** - This generates a device specific status JSON for use by the device's webbased control panel.
* Functions managing device setting persistent storage. (Note: the library manages EEPROM storage for a character array with MAXCUSTOMDATALEN characters in it for custom use in devices to save persistent settings or status information)
  - **void initializeDeviceCustomData()** - This sets up device specific data elements on boot up if nothing is found stored in EEPROM 
  - **void extractDeviceCustomData()** - This extracts device specific data from a string stored in EEPROM 
  - **void encodeDeviceCustomData()** - This encodes device speciifc data into a string stored in EEPROM
  - **void showEEPROMdevicePersonalityData()** - This display device type specific personality data for debugging
  
## Program flow explained in more detail.
Coming...




