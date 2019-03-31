# fishyDIYdevices - Limit Switch Actuator Example
* For detailed project instructions to include 3D printing files, wiring diagrams, hardware details, etc, go to [fishyDIY.com](http://fishyDIY.com).  Those complete project instructions are a work in progress.  
* You can build and upload the software on a ESP8266 chip alone (no real device) to see/test the basic web interface and WiFi configuration features by following these steps.
## Basic steps to build this example:
### 1. Get the Arduino IDE set up (go to Step 2 if you already have the fishyDevice library added to your IDE):
  #### a. Get the [Ardunio IDE](https://www.arduino.cc/). Note: don't get the Windows 10 APP version by mistake - get the full IDE.
  #### b. Add a link to the ESP8266 board manager:	
  - In the IDE go to “File>Preferences” (or hit Ctrl-comma):
  - By Additional Boards Manager URLs: enter http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Click “OK”
  #### c. Select board package version 2.3.0 (this is stable and works well):
  - In the IDE go to “Tools>Board>Board Manager…”
  - Click on “Filter your search” and type “ESP8266”
  - Click on esp8266 by ESP8266 Community, select version 2.3.0, and press “Install”
  - When installed, in the IDE go to “Tools>Board” the scroll down and select: “NodeMCU 1.0 (ESP-12E Module)”
  #### d. Add the fishyDIYdevices library and dependencies:
  - If you don't have them already, click on "ZIP" for each of the 5 libraries in the table below. 
  - Install each of the 5 libraries in the Arduino IDE using the option under "Sketch > Include Library > Add .ZIP Library...".
  
|Device|Library|Repository|ZIP|
|-|-|-|-|
|ESP8266|**fishyDIYdevices** by Stephen Fisher ESP8266|[GIT](https://github.com/sefisher/fishyDIYdevices)|[ZIP](https://github.com/sefisher/fishyDIYdevices/archive/Main.zip)|
|ESP8266|**ESPAsyncTCP** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncTCP)|[ZIP](https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip)|
|ESP8266|**ESPAsyncWebServer** by Hristo Gochkov ESP8266|[GIT](https://github.com/me-no-dev/ESPAsyncWebServer)|[ZIP](https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip)|
|ESP8266|**FauxmoEsp** by Xose Perez ESP8266|[GIT](https://bitbucket.org/xoseperez/fauxmoesp)|[ZIP](https://bitbucket.org/xoseperez/fauxmoesp/get/f60c46d80f9b.zip)|
|ESP8266|**AccelStepper** by Mike McCauley|[PAGE](http://www.airspayce.com/mikem/arduino/AccelStepper/index.html)|[ZIP](http://www.airspayce.com/mikem/arduino/AccelStepper/AccelStepper-1.59.zip)|

### 2. Compile and upload the software to your device:
  #### a. Open "fishyDIYdevices.zip" and extract the example folder named "FD-Limit-Switch-Actuator" from  (.zip > examples > LimitSwitchActuator > FD-Limit-Switch-Actuator). You should find the following files in the extracted folder:
  - [FD-Limit-Switch-Actuator.ino](FD-Limit-Switch-Actuator/FD-Limit-Switch-Actuator.ino) - this is the main file with loop() and setup() functions, it is generally the same flow for all fishyDIYdevices. It also contains all the definitions for the custom (device type specific) functions.
  - [FD-Limit-Switch-Actuator.h](FD-Limit-Switch-Actuator/FD-Limit-Switch-Actuator.h) - this header adds declarations for functions, adds device-type specific variables, settings, and global variables.
  - [FD-Device-Definitions.h](FD-Limit-Switch-Actuator/FD-Device-Definitions.h) - this header defines device settings for compiling (name, device-type, etc), it is common to all fishyDIYdevices. You can compile it as is and make the changes to the device settings via the web interface.
  #### b. Build and upload the file:
  - Open "FD-Limit-Switch-Actuator.ino" in the Arduino IDE
  - Make sure your ESP8266 is connected to your computer via a USB and select its port in the IDE (Tools > Port > 'COM3,COM8, etc')
  - Build and upload the sketch using "Sketch > Upload"
  - NOTE: Do a hard RESET on the device (power cycyle or press the RST button on the chip) after you successfully Upload the first time
  #### c. Connect to the device and setup WiFi:
  - Connect your device to a power source via USB (recommend not using your computer USB to ensure a stable voltage for the AP mode, if you try it and can't connect to the network below then try powering it from a regular plug-in USB power source).
  - It will boot in AP mode the first time with the name "New Device" (or whatever you named it in FD-Device-Definitions.h).
  - Go to your phone's or computer's WiFi settings and find the "New Device" network and try to connect to it.
  - The AP password will be "123456789" (or whatever you set up in FD-Device-Definitions.h)
  - Once connected to the device's network, open "http://192.168.1.4" in your web browser and enter the Wifi SSID and password for your local network.
### 3. Test your device:
  #### a. Go to [http://fishydiy.local/](http://fishydiy.local/) (or the IP address your router provided the new device if your network doesn't support mDNS).
  #### b. If you think it will work for you, set up the hardware.
## Hardware Setup:
### Parts:
 - ESP8266 (NodeMCU ESP 12E)
 - 28BYJ-48 Stepper Motor
 - ULN2003 Motor Driver
 - 2 limit switches (normally open contacts, momentary)
### Wiring:
![Wiring](extras/wiring%20diagram.png)
## Thanks
Thanks to Anne Elise Bolton for helping test and edit these instructions!
