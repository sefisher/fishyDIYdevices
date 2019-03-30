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
 5) Mapview **under development** to see and control your devices overlaid on a layout/map of your home. (Logging, monitoring, and controlling in this way uses a separate monitoring device like a raspberry pi).
 
## Dependencies

Besides the libraries already included with the Arduino Core for ESP8266, these libraries are also required:

FauxmoEsp by [Xose Perez](http://tinkerman.cat/):
* [fauxmoesp](https://bitbucket.org/xoseperez/fauxmoesp) - tested and developed with version 3.0.0

ESPAsyncTCP and ESPAsyncWebServer by [me-no-dev](https://github.com/me-no-dev) (Hristo Gochkov):
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) library
* [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) library.

### Arduino IDE

If you are using the Arduino IDE you will need to install the required library from sources. If you are just getting started it is easiest to download the library as a ZIP file and install it using the option under "Sketch > Include Library > Add .ZIP Library...".

You can look for it manually but I have gathered the URL here for convenience:

|Device|Library|Repository|ZIP|
|-|-|-|-|
|ESP8266|**ESPAsyncTCP** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncTCP)|[ZIP](https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip)|
|ESP8266|**ESPAsyncWebServer** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncWebServer)|[ZIP](https://github.com/me-no-dev/AsyncTCP/archive/master.zip)|
|ESP8266|**FauxmoEsp** by Xose Perez ESP8266|[GIT](https://github.com/simap/fauxmoesp)|[ZIP](https://github.com/simap/fauxmoesp/archive/master.zip)|

## Examples
* Detailed build instructions for the example devices (including files for 3D printed parts, hardware lists, etc) will be available at [fishyDIY.com](http://fishydiy.com/). 
* The examples provided can generally be compiled and uploaded as is if you use the same electronic hardware. There are template html files in each example directory provided in case you want to customize the interfaces or make your own new device type. **Please note: SPIFFS is NOT used, all the device needs is in a single binary uploaded by the Arduino IDE, the HTML files are for you to develop your own look/feel/functions if desired.
The following built-in device types are provided with hardware examples:
    - Limit-SW-Actuator (limit switches allow moving between full open and shut, or positioning anywhere from 0-100% of that range). This is a pretty flexible device that can be set up mechanical for a linear actuator (moving things back and forth) or rotating.  Using 2 limit switches makes positioning reliable since it can regain track on position if the motor ever "slips" (tries to move a step but doesn't have enough force).
* Uses 28BYJ-48 stepper motor with ULN2003 motor driver
Examples for the following types are under development:
    - RGBLED controller (colors and dimming control for a low power RGB LED strip)
    - Light switch controls (for replacing normal toggle light switches).
    - Infrared motion sensors (passive sensors to detect motion and trigger events).
    - Temperature sensors (just
    - Single-SW-Actuator (single limit switch allow resetting motion tracking from a single known point (limit SW) positioning anywhere fromm 0-100% with 100% set by software and conuting motor rotations). These can be used for the same things as a Limit-SW-actuator where adding a second switch is hard (or ugly) - like blinds. But it is slightly less reliable than a full limit switch actuator with two switches. 

## Security
From a security standpoint - fishyDevices are intended to operate on a home WiFi network with device-device data transfer protected by your properly secured router (using the router's security to have device to device comms encrypted at the link layer). Each device can be controlled using a self-served control panel using any webbrowser only on your local network. Remote control (from outside your local network) is only enabled through your Alexa devices and Amazon.
