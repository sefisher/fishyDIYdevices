# fishyDIYdevices - Limit Switch Actuator Example
For detailed project instructions to include 3D printing files, wiring diagrams, hardware details, etc, go to [fishyDIY.com](http://fishyDIY.com).  Those complete project instructions are a work in progress.  
* You can build and upload the software on a ESP8266 chip alone (no real device) to see/test the basic web interface and WiFi configuration features by following these steps.
## Basic steps to build this example:
### 1. Get the Arduino IDE set up:
#### * Get the [Ardunio IDE](https://www.arduino.cc/). Note: don't get the Windows 10 APP version by mistake - get the full IDE.
#### * Add a link to the ESP8266 board manager:	
  - In the IDE go to “File>Preferences” (or hit Ctrl-comma):
  - By Additional Boards Manager URLs: enter http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Click “OK”
#### * Select board package version 2.3.0 (this is stable and works well):
  - In the IDE go to “Tools>Board>Board Manager…”
  - Click on “Filter your search” and type “ESP8266”
  - Click on esp8266 by ESP8266 Community, select version 2.3.0, and press “Install”
  - When installed, in the IDE go to “Tools>Board” the scroll down and select: “NodeMCU 1.0 (ESP-12E Module)”
#### * Add the fishyDIYdevices library and dependencies (see main README.md)
### 2. Compile and upload to your device:
#### * Download the 5 files in the example folder into a directory named "FD-Limit-Switch-Actuator"
  - [FD-Limit-Switch-Actuator.ino](https://github.com/sefisher/fishyDIYdevices/blob/Main/examples/LimitSwitchActuator/FD-Limit-Switch-Actuator.ino) - this is the main file with loop() and setup() functions, it is generally the same flow for all fishyDIYdevices.
  - [FD-Device-Definitions.h](https://github.com/sefisher/fishyDIYdevices/blob/Main/examples/LimitSwitchActuator/FD-Device-Definitions.h) - this header defines device settings for compiling (name, device-type, etc), it is common to all fishyDIYdevices. You can compile it as is and make the changes to the device settings via the web interface.
  - [FD-LSA-Custom-Functions.ino](https://github.com/sefisher/fishyDIYdevices/blob/Main/examples/LimitSwitchActuator/FD-LSA-Custom-Functions.ino) - this contains the device-type specific function defintions to operate the limit-switch-actuator.
  - [FD-LSA-Globals.h](https://github.com/sefisher/fishyDIYdevices/blob/Main/examples/LimitSwitchActuator/FD-LSA-Globals.h) - this header adds device-type specific variables, settings, and global variables.
  - [FD-LSA-Web-Resources.h](https://github.com/sefisher/fishyDIYdevices/blob/Main/examples/LimitSwitchActuator/FD-LSA-Web-Resources.h) - this header provides the string for compiling the device's web control interface.
#### * Build and upload the file
  - Open "FD-Limit-Switch-Actuator.ino" within the Ardnuino IDE
  - Make sure your ESP8266 is connected to your computer via a USB and select its port in the IDE (Tools > Port > 'COM3,COM8, etc')
  - Build and upload the sketch using Sketch > Upload
  - NOTE: Do a hard RESET on the device (power cycyle or press the RST button on the chip) after you successfully Upload the first time
#### * Connect to the device and setup WiFi
  - Connect your device to a power source via USB (recommend not using your computer USB to ensure a stable voltage for the AP mode, if you try it and can't connect to the network below then try powering it from a regular plug-in USB power source).
  - It will boot in AP mode the first time with the name "New Device" (or whatever you named it in FD-Device-Definitions.h).
  - Go to your phone's or computer's WiFi settings and find the "New Device" network and try to connect to it.
  - The AP password will be "123456789" (or whatever you set up in FD-Device-Definitions.h)
  - Once connected to the device's network, open "http://192.168.1.4" in your web browser and enter the Wifi SSID and password for your local network.
### 3. Operate your device:
#### * Go to [http://fishydiy.local/](http://fishydiy.local/) (or the IP address your router provided the new device if your network doesn't support mDNS).
#### * Have fun!
  
