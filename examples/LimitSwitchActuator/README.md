# fishyDIYdevices - Limit Switch Actuator Example
For detailed project instructions to include 3D printing files, wiring diagrams, hardware details, etc, go to [fishyDIY.com](http://fishyDIY.com).  These project instructions are a work in progress.
## Basic steps to build this example:
* Get the [Ardunio IDE](https://www.arduino.cc/). Note: don't get the Windows 10 APP version by mistake - get the full IDE.
* Add a link to the ESP8266 board manager:	
- In the IDE go to “File>Preferences” (or hit Ctrl-comma):
- By Additional Boards Manager URLs: enter http://arduino.esp8266.com/stable/package_esp8266com_index.json
- Click “OK”
* Select board package version 2.3.0 (this is stable and works well):
- In the IDE go to “Tools>Board>Board Manager…”
- Click on “Filter your search” and type “ESP8266”
- Click on esp8266 by ESP8266 Community, select version 2.3.0, and press “Install”
- When installed, in the IDE go to “Tools>Board” the scroll down and select: “NodeMCU 1.0 (ESP-12E Module)”
* Add the fishyDIYdevices library and dependencies (see main README.md)
* Download the 5 files in the example folder into a directory named "FD-Limit-Switch-Actuator"
- FD-Limit-Switch-Actuator.ino - this is the main file with loop() and setup() functions, it is generally the same flow for all fishyDIYdevices.
- FD-Device-Definitions.h - this header defines device settings for compiling (name, device-type, etc), it is common to all fishyDIYdevices.
- FD-LSA-Custom-Functions.ino - this contains the device-type specific function defintions to operate the limit-switch-actuator.
- FD-LSA-Globals.h - this header adds device-type specific variables, settings, and global variables.
- FD-LSA-Web-Resources.h - this header provides the string for compiling the device's web control interface.


