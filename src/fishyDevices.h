/*
 fishyDevices - A fishyDIY.com library used to define interoperating home automation devices deployed on cheap Internet of Things (IoT) chips. It is designed to work on ESP8266 (these are open source chip designs also known as Esperrif ESP8266, NodeMCU, and packaged under a variety of manufacture names). This library is set up to use the Arduino programming language. It makes extensive use of Hristo Gochkov's ESP Async toolset and Xose Pérez's fauxoESP library.

 From a security standpoint - fishyDevices are intended to operate on a home WiFi network with device-device data transfer protected by your properly secured router (using the router's security to have device to device comms encrypted at the link layer). Each device can be controlled using a self-served control panel using any webbrowser only on your local network. Remote control (from outside your local network) is only enabled through your Alexa or Google Home app on your phone.

 The library takes care of  the following basic functions:
 1) Setting/changing/storing WiFi crendtials without a wired connection.
 2) Providing over the air software updating (inintial SW load on any device is via usb at your computer).
 3) Uses fauxmoESP to create an Alexa or Google Home voice control interface for controlling the devices.
 4) Display a list and mapview (**mapview is for monitoring,logging, and controlling on a separate monitoring device like a raspberry pi) of all fishyDevices on your network with a brief status displayed.
 5) The following built-in device types are provided with hardware examples:
    - Limit-SW-Actuator (dual limit switches allow moving between two states, or positioning anywhere fromm 0-100% of that range). These can be used for things ranging from blind controls, ventilation damper positioners, rotating something on display, etc. 
    - Simple-Switches (on/off switches for light switch panels {upto 4 switches in current design with settings for wither pushbuttons or touch-type switches, can use builtin LED or a spearate LED for blinking indications of button press or other functions})
 6) Blank templates are provided for making your own custom fishyDevice types.  The following types are under devlopment:
    - Infrared motion sensors
    - Dimming lights
    - Temperature sensors
    - RGBLED strip controls
    - Single-SW-Actuator (single limit switch allow resetting motion tracking from a single known point (limit SW) positioning anywhere fromm 0-100% with 100% set by software and conuting motor rotations). These can be used for the same things as a Limit-SW-actuator where adding a second switch is hard (or ugly) - like blinds. But it is slightly less reliable than a full limit switch actuator with two switches. 

Copyright (C) 2020 by Stephen Fisher 

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

For Fauxmo Library - Copyright (c) 2018 by Xose Pérez <xose dot perez at gmail dot com> (also under MIT License) - see below for full licence.
For ESPAsync Toolset - Copyright (c) 2016 Hristo Gochkov (under GNU License version 2.1). All rights reserved.
*/

//FISHYDEVICE DEBUG FLAGS:
//#define DEBUG_MESSAGES true      //debugging for general device problems 
//#define DEBUG_UDP_MESSAGES true  //debugging for network comms 
//#define UDP_PARSE_MESSAGES true  //debugging for parsing messages 
//#define DEBUG_WIFI_MESSAGES true //debugging WIFI connection issues
//FUAXMOESP DEBUG FLAGS:
//#define DEBUG_FAUXMO                Serial
//#define DEBUG_FAUXMO_VERBOSE_TCP    true
//#define DEBUG_FAUXMO_VERBOSE_UDP    true

#ifndef fishyDevice_h
#define fishyDevice_h

//Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> 
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <EEPROM.h>

//define the maximum length for the text used as commands (for serial comms or in URLs as GETs)
//CHANGING THESE IMPACTS EEPROM STORAGE AND REQUIRES REINITIALIZATION OF EVERY DEVICE - CHANGE WITH CARE
#define MAXCMDSZ 300
#define MAXNAMELEN 41
#define MAXTYPELEN 21
#define MAXNOTELEN 56
#define MAXCUSTOMDATALEN 256
#define EEPROMsz 700 //(see calculated size below - fD.myEEPROMdata struct; 673 req'd, set at 700 for some margin)

#define ESP8266                 //tell libraries this is an ESP8266 (vice ESP32)
#define SW_UPDATE_DELAY 10      //the number of seconds to wait for a reboot following a software update - increase if you see "page not found errors" after an update

//Board communication rate and UDP port number
#define SERIAL_BAUDRATE 115200
#define UDP_LOCAL_PORT 8266
#define HOME_MONITOR_PORT 8080

//DNS Server Port
#define DNS_PORT 53

//NETWORK SETTINGS
#define MAX_DEVICE 30 //sets how many fishyDevices can be tracked by the MASTER

//UDP MESSAGE STRINGS" - customizable strings used in UDP comms between devices and MASTER
//if these are altered and you are using the homeMonitor package - update the 
//commands in the configureHome.html and piMonitor.py files as well
#define UDP_ACTIVITY "~UDP~ACTIVITY_MESSAGE" //a device reporting activity
#define UDP_MSG4LOG "~UDP~MESSAGE_4_LOGGER" //MASTER telling LOGGER something
#define UDP_YO "~UDP~ANYFISHYDEV_THERE" //MASTER asking all devices to report in
#define UDP_MASTER "~UDP~FISHYDIYMASTER" //MASTER asking all devices to report in
#define UDP_HERE "~UDP~POLL_RESPONSE" //devices reporting in to MASTER
#define UDP_LOGGER "~UDP~FISHYDIYLOGGER" //New LOGGER online
#define UDP_CONFIG_REQUEST "~UDP~CONFIGREQUEST" //Requesting device config string
#define UDP_CONFIG_RESPONSE "~UDP~FISHYDIYCONFIG" //Provide device config string
#define UDP_ACK_LOGGER "~UDP~HEARD_NEW_LOGGER" //acknowledge logger
#define UDP_NEWLOCATION "~UDP~LOCATION_CHANGE" //new device location
#define UDP_LOGGER_RCVD "~UDP~LOGGER_RCVD_DATA" //logger received and parsed its message


//=======================================================================
//  FAUXMOESP SECTION
//=======================================================================
#define USERNAME "t4bK6SilLBy4DutQo0P8yORztUFrAf5-NDTi7wFq"
#define FAUXMO_UDP_MULTICAST_IP     IPAddress(239,255,255,250)
#define FAUXMO_UDP_MULTICAST_PORT   1900
#define FAUXMO_TCP_MAX_CLIENTS      10
#define FAUXMO_TCP_PORT             1901
#define FAUXMO_RX_TIMEOUT           3

#ifdef DEBUG_FAUXMO
    #if defined(ARDUINO_ARCH_ESP32)
        #define DEBUG_MSG_FAUXMO(fmt, ...) { DEBUG_FAUXMO.printf_P((PGM_P) PSTR(fmt), ## __VA_ARGS__); }
    #else
        #define DEBUG_MSG_FAUXMO(fmt, ...) { DEBUG_FAUXMO.printf(fmt, ## __VA_ARGS__); }
    #endif
#else
    #define DEBUG_MSG_FAUXMO(...)
#endif

#ifndef DEBUG_FAUXMO_VERBOSE_TCP
    #define DEBUG_FAUXMO_VERBOSE_TCP    false
#endif

#ifndef DEBUG_FAUXMO_VERBOSE_UDP
    #define DEBUG_FAUXMO_VERBOSE_UDP    false
#endif

#if defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <AsyncTCP.h>
#else
	#error Platform not supported
#endif

#include <WiFiUdp.h>
#include <functional>
#include <vector>


typedef std::function<void(unsigned char, const char *, bool, unsigned char)> TSetStateCallback;

typedef struct {
    char * name;
    bool state;
    unsigned char value;
} fauxmoesp_device_t;

class fauxmoESP {

    public:

        ~fauxmoESP();

        unsigned char addDevice(const char * device_name);
        bool renameDevice(unsigned char id, const char * device_name);
        bool renameDevice(const char * old_device_name, const char * new_device_name);
        bool removeDevice(unsigned char id);
        bool removeDevice(const char * device_name);
        char * getDeviceName(unsigned char id, char * buffer, size_t len);
        int getDeviceId(const char * device_name);
        void onSetState(TSetStateCallback fn) { _setCallback = fn; }
        bool setState(unsigned char id, bool state, unsigned char value);
        bool setState(const char * device_name, bool state, unsigned char value);
        bool process(AsyncClient *client, bool isGet, String url, String body);
        void enable(bool enable);
        void createServer(bool internal) { _internal = internal; }
        void setPort(unsigned long tcp_port) { _tcp_port = tcp_port; }
        void handle();

    private:

        AsyncServer * _server;
        bool _enabled = false;
        bool _internal = true;
        unsigned int _tcp_port = FAUXMO_TCP_PORT;
        std::vector<fauxmoesp_device_t> _devices;
		#ifdef ESP8266
        WiFiEventHandler _handler;
		#endif
        WiFiUDP _udp;
        AsyncClient * _tcpClients[FAUXMO_TCP_MAX_CLIENTS];
        TSetStateCallback _setCallback = NULL;

        String _deviceJson(unsigned char id);

        void _handleUDP();
        void _onUDPData(const IPAddress remoteIP, unsigned int remotePort, void *data, size_t len);
        void _sendUDPResponse();

        void _onTCPClient(AsyncClient *client);
        bool _onTCPData(AsyncClient *client, void *data, size_t len);
        bool _onTCPRequest(AsyncClient *client, bool isGet, String url, String body);
        bool _onTCPDescription(AsyncClient *client, String url, String body);
        bool _onTCPList(AsyncClient *client, String url, String body);
        bool _onTCPControl(AsyncClient *client, String url, String body);
        void _sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime);

};
//================================================================================================================


//TODO Cleanup: remove the comments below after versifying functions with Limt Switch Actuator
//extern const bool OPEN_IS_CCW;
//extern const bool SWAP_LIM_SW;

//DEVICE SETTINGS - these are defined in a user .h file (e.g., FD-Device-Definitions.h)
//For the MASTER NODE (webserver node) this sets the number of devices it can manage on the net
extern const char CUSTOM_DEVICE_NAME[];
extern const bool MASTER_NODE;
extern const int  DEVICE_TIMEOUT;
extern const char CUSTOM_NOTE[];
extern const char SW_VER[];
extern const char CUSTOM_DEVICE_TYPE[];
extern const char SOFT_AP_PWD[]; 
extern const char INITIALIZE[];
extern const bool USE_SERIAL_INPUT;
extern const bool DO_BLINKING;
extern const int BLINK_LED;
extern const bool FAUXMO_ENABLED;

//Test switches for Serial output text (set to false to disable debug messages) 
#ifndef DEBUG_MESSAGES
    #define DEBUG_MESSAGES false      //debugging for general device problems (movement, switches, etc)
#endif
#ifndef DEBUG_UDP_MESSAGES
    #define DEBUG_UDP_MESSAGES false  //debugging for network comms (MASTER - SLAVE issues with nodes on the network)
#endif
#ifndef UDP_PARSE_MESSAGES
    #define UDP_PARSE_MESSAGES false  //debugging for parsing messages - used after you've changed the message structures
#endif
#ifndef DEBUG_HEAP_MESSAGE
    #define DEBUG_HEAP_MESSAGE false  //just tracking the heap size for memory leak issues or overloaded nodeMCUs
#endif
#ifndef DEBUG_WIFI_MESSAGES
    #define DEBUG_WIFI_MESSAGES false  //shows wifi connection debugging info
#endif

//A typedef struct of type fishyDevice to hold data on devices on the net; and
//then create an array of size MAX_DEVICE to store all the stuff found on the net
//IF YOU EDIT THIS STRUCT ALSO UPDATE myEEPROMdata (TO SAVE INFO) AND
//REVIEW FUNCTIONS "UDPpollReply()" and "UDPparsePollResponse()" and "getNodeJSON()" in fishyDevices.cpp
typedef struct fishyDeviceData
{
    IPAddress ip;
    String name = "";
    String typestr = "";
    String statusstr = "";
    bool inError = false; //captures timeout, not being calibrated, and any device specifc errors
    bool isMaster;
    String shortStat = "";
    int locationX = -1;
    int locationY = -1;
    int locationZ = -1;
    bool dead = true;
    unsigned long timeStamp = 0; //used to track when device updates were made last to cull dead nodes
};

typedef struct EEPROMdata //676 bytes
{
    char initstr[13] = "";                        //13 bytes
    char namestr[MAXNAMELEN] = "";                //41 bytes
    bool master = false;                          //1 byte
    char typestr[MAXTYPELEN] = "";                //21 bytes
    char note[MAXNOTELEN] = "";                   //56 bytes
    char swVer[11] = "";                          //11 bytes
    int timeOut = 60;                             //4 bytes
    bool deviceTimedOut = false;                  //1 byte
    char deviceCustomData[MAXCUSTOMDATALEN] = ""; //256 bytes - 255 characters - format: '{name=value&name=value&name=value}' (no spaces, no "&", no "=" stored in string)
    bool blink_Enable = false;                     //1 byte
    bool fauxmo_Enable = false;                    //1 byte

    //storage for future use--------------------------------
    //reserved since changing EEPROM layout requires more work to 
    //update fielded devices
    char reserved_data_package[MAXCUSTOMDATALEN-1]; //255 bytes - 254 characters 
    //-----------------------------------------------------

    int locationX = -1;                           //x position on level picture  //4 bytes
    int locationY = -1;                           //y position on level picture  //4 bytes
    int locationZ = -1;                           //level number	  		     //4 bytes
};

//Struct for WiFi settings data
typedef struct wifiConnect
{
    const char *softAP_ssid;
    const char *softAP_password;
    char ssid[32] = "";
    char password[32] = "";
    boolean connect;
    boolean softAPmode;
    int status = WL_IDLE_STATUS;
    int connectTryCount = 0;
};

//-----------------------------------------------------------------------------

class fishyDevice
{

  public:
    fishyDevice(const char *); // Constructor

    /*-------------------------------------------------------------------------
     These functions are DECLARED here but MUST BE DEFINED IN THE CUSTOM DEVICE's .ino file since they are unique to each device type (they can be defined to do nothing if appropriate, but need to be defined there to compile)
    */

    void operateDevice(); //this is run very loop cycle to make the device work based on user input
    void deviceSetup(); //this is run at initialization (from setup()) to startup the device
    bool executeDeviceCommands(char inputMsg[MAXCMDSZ*2], IPAddress remote);//run by executecommands first to allow device specific commands to be processed (can override built-in commands processes as well)
    void executeState(unsigned char device_id, const char* device_name, bool state, unsigned char value, int context); //execute voice command state changes
    void UDPparseConfigResponse(char inputMsg[MAXCMDSZ*2], IPAddress remote); //parse configuration string data to update all the stored parameters
    String getConfigString(); //generate a configuration string that can be sent back and processed by UDPparseConfigResponse to refresh all the stored parameters
    String getStatusString(); //return a string with the device's status for display
    String getShortStatString(); //return a SHORT string (3 char max) summarizing the device's status for min display
    String getHabridgeControls(); //return a string with control information to be used in ha-bridge device.db
    void initializeDeviceCustomData(); //setup device specific data elements on boot up if not stored in EEPROM
    void extractDeviceCustomData(); //extract device data from a string stored in EEPROM
    void encodeDeviceCustomData();  //encode device data into a string stored in EEPROM
    void showEEPROMdevicePersonalityData();  //display device type specific personality data
    bool isCustomDeviceReady(); //report if the device is ready, if not sets Error flag 
    String getDeviceSpecificJSON(); //generate a device specific status JSON for use by the device's webbased control panel
    /*-----------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------*/
    //The remaining functions are defined in fishyDevices.cpp

    //startup and initialization functions
    void FD_setup();
    void initializePersonalityIfNew();
    void showEEPROMPersonalityData();
    void showThisNode(fishyDeviceData holder);
    void WifiFauxmoAndDeviceSetup();
    void showHeapAndProcessSerialInput();
    void serialStart();
    void checkResetOnLoop();
    void showPersonalityDataSize();
    void resetController();
    unsigned char addAnotherDevice(const char *device_name);
    //take and process commands (from UDP/websocket)
    void executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote, int client_id); 
    void updateLocation(char inputMsg[MAXCMDSZ]);
    //general helper functions
    void setBlinkerLEDPin(int LEDPin);
    void fastBlinks(int numBlinks);
    void slowBlinks(int numBlinks);
    void doBlinking(void);
    String paddedH3Name(String name);
    String threeDigits(int i);
    String paddedInt(int lengthInt, int val);
    String paddedIntQuotes(int lengthInt, int val);
    // UDP Communication functions
    void UDPprocessPacket();
    void UDPkeepAliveAndCull();
    void announceReadyOnUDP();
    void UDPbroadcast();
    void UDPackLogger();
    void UDPconfigRequest();
    void UDPannounceMaster();
    void UDPpollReply(IPAddress remote);
    void UDPparsePollResponse(char inputMsg[MAXCMDSZ], IPAddress remote);
    void UDPhandleActivityMessage(char inputMsg[MAXCMDSZ], IPAddress remote);
    String UDPmakeActivityMessage(String message);
    // functions to maintain list of active fishyDevices on network
    fishyDeviceData makeMyfishyDeviceData();
    int dealWithThisNode(fishyDeviceData netDevice);
    int findNode(IPAddress lookupIP);
    int findDeadNode();
    void updateNode(int index, fishyDeviceData updatedDevice);
    int storeNewNode(fishyDeviceData newDevice);
    void cullDeadNodes();
    // EEPROM Management functions
    void storeDataToEEPROM();
    void retrieveDataFromEEPROM();
    bool loadCredentials();
    void saveCredentials();
    //wifi and webserver and websock functions
    //-wifi
    void WiFiSetup();
    void connectWifi();
    String readStatus(int s);
    void manageConnection();
    void onSetStateForFauxmo(unsigned char, const char *, bool, unsigned char);
    //-websock
    void webSocketEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void updateClients(String message);
    void updateClients(String message, bool forceUpdate);
    void updateSpecificClient(String message, int client_id);
    //-webserver
    void handleNetworkJSON(AsyncWebServerRequest *request);
    void handleNodeJSON(AsyncWebServerRequest *request);
    void handleConfigString(AsyncWebServerRequest *request);
    void handleHabridgeString(AsyncWebServerRequest *request); 
    void handleNotMaster(AsyncWebServerRequest *request);
    String getNetworkJSON();
    String getNodeJSON();
    String urlencode(String str);
    void handleRoot(AsyncWebServerRequest *request);
    void handleWifi(AsyncWebServerRequest *request);
    void handleWifiUpdater(AsyncWebServerRequest *request);
    void handleWifiSave(AsyncWebServerRequest *request);
    void handleJustReboot(AsyncWebServerRequest *request);
    void handleCtrl(AsyncWebServerRequest *request);
    void handleSWupdateDevForm(AsyncWebServerRequest *request);
    void handleCSS(AsyncWebServerRequest *request);
    void handleJS(AsyncWebServerRequest *request);
    void handleSWupdateDevPost(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleSWupdateDevPostDone(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    void handleOnRequestBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void resetWiFiCredentials();
    void runNormalServer();
    void runSoftAPServer();
    void checkWifiStatus();
    
    const char *webctrlstrPtr;        //used for passing the WEBCTRLSTR constant stored in the device unique (custom) web resource file
    unsigned long deviceResponseTime; //used for tracking how long the device is being moved to determine if TIMEOUT ERROR is needed

    //struct for storing personailty data in real time and for storing in EEPROM
    //remember a character is needed for the string terminations
    //SW reports needed 676 bytes; left some margin
    EEPROMdata myEEPROMdata;

    fauxmoESP fauxmo;          //fauxmo device for alexa interactions
    wifiConnect myWifiConnect; //global used for managing wifi connection
    WiFiEventHandler gotIpEventHandler;
    
    // apIP and netMsk are Soft AP network parameters
    IPAddress apIP; //ip address for device served AP - serves a webpage to allow entering wifi credentials from user
    IPAddress netMsk;
    IPAddress masterIP; //the IP address of the MASTER device (if set)
    IPAddress loggerIP = IPAddress(0, 0, 0, 0); //the IP address of the logging device (if set)
    String loggerPagePort = "";

    //make the webserver and web updater
    AsyncWebServer *httpServer; //for master node web server
    AsyncWebSocket *webSocket;  //for web control panel to device fast communications
    DNSServer *dnsServer;       //for AP control

    WiFiUDP Udp; //for UDP traffic between nodes

    fishyDeviceData deviceArray[MAX_DEVICE]; //array of fishydevices for use in creating contorl panel webpages

    //variables for asynchronous blinking
    unsigned long fastBlinkStart;
    unsigned long slowBlinkStart;
    int fastBlinkState = HIGH;
    int slowBlinkState = HIGH;
    int fastBlinkCount = 0;
    int slowBlinkCount = 0;
    bool resetOnNextLoop; //used to tell the device to reset after it gets to the next main operating loop
    String _updaterError; //used for tracting SW uploading errors
    bool use_serial;
    int blinker_led;
};

/*----------------------------------------------------------------------------
->This following constants contain the common fishyDevice HTML served by the webserver that is minified and converted to a string for compliation.
->See SOURCE FILE notes below to find the source HTML file that is readable with instructions to update these strings.
-----------------------------------------------------------------------------*/

/*===========================================================================*/
// PAGE STYLES - used for multiple pages, provided in respose to /styles.css
//SEE SOURCE FILE FOR CHANGES/TESTING IN fishyDIYdevices/extras/html/styles.css
static const char WEBSTYLESSTR[] PROGMEM = ":root{font-family: Arial, Helvetica, sans-serif;font-style: normal;--cp-sz: 290px;--big-fnt: 25px;--mid-fnt: 20px;--sml-fnt: 15px;--bg-col: #16234b;--bg-col-ack: rgb(43, 206, 97);--drk-col: #3366cc;--btn-col: #ccc;--btn-colR: rgb(151, 90, 90);--btn-colY: rgb(198, 223, 111);--btnBorder-col: rgb(96, 109, 128);--lght-col: #ffffff;--cfg-lab: 280;--cfg-inp: 180;--lab-sz: 90px;--val-sz: 65px;--sld-sz: 125px;--rad-sz: 10px;--hdr-ht: 50px;--smrad-sz: 7px;--thm-sz: 25px;--sld-ht: 10px;--sld-mar: 5px;--sldRow-mar: 4px;--sldRow-ht: 30px;--btnRow-mar: 2px;--btnRow-ht: 30px;--btn-ht: 28px}a{color: var(--lght-col)}body{padding: 5px;background: var(--bg-col);transition: .5s}.flex-container{display: flex;flex-wrap: wrap;justify-content: center;}.flex-container>div{border-radius: var(--rad-sz);background-color: var(--lght-col);width: calc(var(--cp-sz)+15px);margin: 5px;vertical-align: middle;text-align: center;line-height: calc(var(--mid-fnt) + 2);font-size: var(--mid-fnt)}.fishyFtr,.fishyFtrSw{border-radius: 0 0 var(--rad-sz) var(--rad-sz);font-size: var(--mid-fnt);color: var(--lght-col);background-color: var(--drk-col);min-height: calc(var(--hdr-ht)/2);text-align: center}.fishyFtrSw{font-size: var(--sml-fnt);font-weight: normal;width: 100%;min-width: var(--cp-sz)}.fishyHdr,.hdrIcon{border-radius: var(--rad-sz) var(--rad-sz) 0 0;font-size: var(--big-fnt);color: var(--lght-col);background-color: var(--drk-col);min-height: var(--hdr-ht)}.fishyHdr{width: 100%;text-align: center}.CP,.CPhdErrClear{width: var(--cp-sz);flex: 0 1 var(--cp-sz);background-color: var(--lght-col)}.CPdevice,.CPdetails{border-radius: 0px;width: var(--cp-sz);flex: 0 1 var(--cp-sz)}.CPdetails{background-color: var(--lght-col);padding:0px;margin:0px;border:none;font-size: var(--sml-fnt)}.CPhd,.CPhd2,.CPhd3,.CPhdErr,.CPft{min-width: calc(var(--cp-sz)+20);color: var(--lght-col);background-color: var(--drk-col);padding:0px;margin:0px;border:none}.CPhd{border-radius: var(--rad-sz) var(--rad-sz) 0px 0px;font-size: var(--big-fnt);-webkit-transition: color 0.4s ease;-moz-transition: color 0.4s ease;-o-transition: color 0.4s ease;transition: color 0.4s ease}.CPft{border-radius: 0px 0px var(--rad-sz) var(--rad-sz);font-size: var(--mid-fnt)}.CPhd3{text-align: center;width: 100%}.CPhdErr{color: red}.CPhdErrClear{color: var(--lght-col)}.flex-grid{display: flex;align-items: center;margin: 2px 0px 2px 0px;justify-content: center}.flex-grid-outer{border-radius: 5px;transition: .5s;display: flex;align-items: center;flex-wrap: wrap;justify-content: center}*{box-sizing: border-box}.button,.bigbutton,.cfgbuttonY,.cfgbuttonR{cursor: pointer;border: 2px solid var(--btnBorder-col);border-radius: 0px;background-color: var(--btn-col);width: calc(var(--cp-sz)/3);min-height: var(--btn-ht);text-align: center}.cfgbuttonR{background-color: var(--btn-colR);width: calc(var(--cp-sz)*.45);font-size: 12px;font-weight: 500}.cfgbuttonY{background-color: var(--btn-colY);width: calc(var(--cp-sz)*.45);font-size: 12px;font-weight: 500}.buttonTxt{text-align: center;font-size: 24px;font-weight: 700}.lilButtonTxt{font-size: 14px;font-weight: 700}.status,.configPanel,.swPanel{border: 2px solid var(--btnBorder-col);border-radius: 5px;background-color: var(--lght-col);width: var(--cp-sz);min-height: 50px;flex-basis: auto;align-content: center;text-align: center;font-family: Arial;font-size: 14px;font-style: normal;font-variant: normal;font-weight: 700;padding: 2px}.sliderDiv{border-radius: 5px;padding: 5px;width: calc(var(--cp-sz)*2/3)}.controlRow{border-radius: 5px;width: calc(var(--cp-sz)-4px);height: 50px}.buttonSet{width: var(--cp-sz)}.info-icon,.info-icon-blink{position: relative;top: -8px;left: 2px;border-radius: 9px;font-size: 30px;font-weight: 800;cursor: pointer;color: var(--lght-col)}.info-icon-blink{font-weight: 800;animation: blinkingText 2.25s infinite}.infoPanel{display:none;text-align: center;border-radius:10px;padding: 5px;background: var(--lght-col);border: var(--drk-col);border-style: solid;border-width: 4px;position:absolute;top:30px;left:50%;width:var(--cp-sz);height:340px;margin-left:calc(-1*var(--cp-sz)/2);z-index: 999}.infoDiv{height: 300px;text-align: left}@keyframes blinkingText{0%{color: red}49%{color: transparent}50%{color: yellow}99%{color: transparent}100%{color: red}}.mainTxtRw{height: 26px;position: relative;left: -10px}.otherTxt{text-align: center;font-size: 20px;font-weight: 500}@media screen and (min-width: 606px){.status{height: 155px}.configPanel{height: 155px}.info-icon,.info-icon-blink{font-size: 28px}}.cfgInp,.cfgInpNum{background-color: var(--lght-col);border: none;padding: 0px;font-size: var(--sml-fnt)}.configPanel{align-content: left;text-align: left;padding: 0px}.cfgInpLab{min-width: var(--cfg-lab);font-size: var(--sml-fnt)}.cfgInp{width: var(--cfg-inp);font-size: var(--sml-fnt)}.cfgInpNum{width: var(--cfg-lab);font-size: var(--sml-fnt)}input[type=range].slider{-webkit-appearance: none;width: 100%;margin: 0.7px 0}input[type=range].slider:focus{outline: none}input[type=range].slider::-webkit-slider-runnable-track{width: 100%;height: 25.6px;cursor: pointer;box-shadow: 1px 1px 1px #000000, 0px 0px 1px #0d0d0d;background: #444;border-radius: 0px;border: 0px solid #010101}input[type=range].slider::-webkit-slider-thumb{box-shadow: 0px 0px 1px #670000, 0px 0px 0px #810000;border: 0px solid #ff1e00;height: 27px;width: 18px;border-radius: 0px;background: rgba(136, 162, 248, 0.93);cursor: pointer;-webkit-appearance: none;margin-top: -0.7px}input[type=range].slider:focus::-webkit-slider-runnable-track{background: #545a5a}input[type=range].slider::-moz-range-track{width: 100%;height: 25.6px;cursor: pointer;box-shadow: 1px 1px 1px #000000, 0px 0px 1px #0d0d0d;background: #484d4d;border-radius: 0px;border: 0px solid #010101}input[type=range].slider::-moz-range-thumb{box-shadow: 0px 0px 1px #670000, 0px 0px 0px #810000;border: 0px solid #ff1e00;height: 27px;width: 18px;border-radius: 0px;background: rgba(136, 162, 248, 0.93);cursor: pointer}input[type=range].slider::-ms-track{width: 100%;height: 25.6px;cursor: pointer;background: transparent;border-color: transparent;color: transparent}input[type=range].slider::-ms-fill-lower{background: #3c4040;border: 0px solid #010101;border-radius: 0px;box-shadow: 1px 1px 1px #000000, 0px 0px 1px #0d0d0d}input[type=range].slider::-ms-fill-upper{background: #484d4d;border: 0px solid #010101;border-radius: 0px;box-shadow: 1px 1px 1px #000000, 0px 0px 1px #0d0d0d}input[type=range].slider::-ms-thumb{box-shadow: 0px 0px 1px #670000, 0px 0px 0px #810000;border: 0px solid #ff1e00;height: 27px;width: 18px;border-radius: 0px;background: rgba(136, 162, 248, 0.93);cursor: pointer;height: 25.6px}input[type=range].slider:focus::-ms-fill-lower{background: #484d4d}input[type=range].slider:focus::-ms-fill-upper{background: #545a5a}.sw,.sw2{position: relative;display: inline-block;top: -6px;width: 38.4px;height: 21.8px}.sw{top: 7px}.sw input{display: none}.swLab{position: relative;top: -7px}.swRow{text-align: right}.swLabHdr{position: relative;top: -6.4px;font-size: var(--sml-fnt)}.sldSw{position: absolute;cursor: pointer;top: 0;left: 0;right: 0;bottom: 0;background-color: #ccc;-webkit-transition: .4s;transition: .4s;border-radius: 21.8px}.sldSw:before{position: absolute;content: '';height: 16.7px;width: 16.7px;left: 2.6px;bottom: 2.6px;background-color: var(--lght-col);-webkit-transition: .4s;transition: .4s;border-radius: 50%}input:checked+.sldSw{background-color: #444}input:focus+.sldSw{box-shadow: 0 0 1px #444}input:checked+.sldSw:before{-webkit-transform: translateX(16.7px);-ms-transform: translateX(16.7px);transform: translateX(16.7px)}.ctrl{height: 250px;width: var(--cp-sz);border: none}.closebtn{position: absolute;z-index: 9999;cursor: pointer;width: 30px;height: 30px;top: 0px;right: 3px;display:none;font-size: 26px}.overlay{height: 0%;width: 100%;position: fixed;top: 0;left: 0;background-color: rgba(255, 255, 255, 0.7);overflow-y: hidden;transition: 0.5s;z-index: 9999;cursor: pointer}.overlay-content{position: fixed;top: 25%;width: 100%;text-align: center;margin-top: 30px}.overlay a{display: block;transition: 0.8s}.modal-body{position: relative;top: 2px;bottom: 2px;left: 2px;right: 2px;width:99%;height:99%;text-align: center}.iframeBody{border: none}.myIframeDiv{border: none;background: transparent}.swUpdate{width: var(--cp-sz);min-height: 300px}";
/*===========================================================================*/

/*===========================================================================*/
//ROOT SITE  - Used to serve up iFrames for each device either for software updates or for controls
//SEE SOURCE FILE FOR CHANGES/TESTING IN fishyDIYdevices/extras/html/CommonRootTemplate.html
static const char WEBROOTSTR[] PROGMEM ="<!doctypehtml><title>fishDIY Device Network</title><meta content=\"width=device-width,initial-scale=1\"name=viewport><script src=/CommonWebFunctions.js></script><link href=/styles.css id=styles rel=stylesheet><script src=CommonWebFunctions.js></script><div class=main id=myBody><script>function addDevice(e){var i=\"<div class='CPdevice' id='CP-\"+e.ip+\"'>\";return i+=addInnerDevice(e),i+=\"</div>\"}function addInnerDevice(e){var i,r;i=\"true\"==e.inError?\"CPhdErr\":\"CPhdErrClear\",r=\"true\"==e.isMaster?\"MASTER:\"+e.ip:e.ip;var n=\"<div class='CPhd' id='hd1-\"+e.ip+\"'>\"+e.name+\"</div>\";return isCtrl?(n+=\"<div id='CPerr-\"+e.ip+\"' class='\"+i+\"'>ERROR</div>\",n+=\"<div >\"+e.statusstr+\"</div>\",n+=\"<button class='CPbutton' onclick='openModal(\\\"\"+e.ip+\"\\\")'; id='myBtn-\"+e.ip+\"'>Open Controls</button>\",n+=\"<div class='CPdetails' >Type:\"+e.typestr+\"</div>\"):n+=\"<iframe id='swUpdate-\"+e.ip+\"' class='swUpdate' src='http://\"+e.ip+\"/SWupdateGetForm' ></iframe><br>\",n+=\"<div class='CPft' id='CPft-\"+e.ip+\"'>\"+r+\"</div>\"}function refreshToggle(){var e=_(\"swRefreshLab-1\");1==_(\"swRefreshChck-1\").checked?(e.innerHTML=\"Auto-refresh On\",pollTimer=setInterval(function(){pollJSON()},1e4)):(e.innerHTML=\"Auto-refresh Off\",clearInterval(pollTimer))}function pollJSON(){var e=\"./network.JSON?nocache=\"+(new Date).getTime();loadJSON(e,function(e){var i=e.logger;_(\"theFtr\").innerHTML=footer(!0,\"\",i);var n,t,a=e.fishyDevices;Array.prototype.forEach.call(a,function(e,i){if(n=_(\"CP-\"+e.ip),\"0\"==e.dead){if(null==n){var r=document.createElement(\"DIV\");r.id=\"CP-\"+e.ip,r.className=\"CPdevice\",n=_(\"flex-container\").appendChild(r)}n.innerHTML=addInnerDevice(e)}else null!=n&&_(\"flex-container\").removeChild(n)});var r=document.querySelectorAll(\".CPdevice\");Array.prototype.forEach.call(r,function(r,e){t=!0,Array.prototype.forEach.call(a,function(e,i){\"CP-\"+e.ip==r.id&&0==e.dead&&(t=!1)}),t&&_(\"flex-container\").removeChild(r)}),console.log(\"loadJSON and page update success\")},function(e){console.log(\"Error with loadJSON\")})}var pollTimer,isCtrl=!0;function buildPage(){alertBadBrowser();var n,t,e=\"./network.JSON?nocache=\"+(new Date).getTime(),a=[],o=_(\"myBody\"),l=document.createElement(\"DIV\");l.className=\"fishyHdr\",o.appendChild(l),\"SWu\"==location.pathname.slice(1,4)?(isCtrl=!1,l.innerHTML=\"fishyDevice Software Update\"):l.innerHTML=\"fishyDevice Controls\",(l=document.createElement(\"DIV\")).className=\"CPhd3\",o.appendChild(l),isCtrl&&(l.innerHTML=\"<span class='swRow'><label class='sw2' id='swRefresh-1'><input type='checkbox' checked id='swRefreshChck-1' onchange=refreshToggle()><span class='sldSw'></span></label><span class='swLabHdr' id='swRefreshLab-1'>Auto-refresh On</span></span>\"),(t=document.createElement(\"DIV\")).className=\"fishyFtr\",t.id=\"theFtr\",o.appendChild(t),t.innerHTML=footer(!0,\"\",\"(IP unset)\"),loadJSON(e,function(e){if(null!=e)loggerIP=e.logger,t.innerHTML=footer(!0,\"\",loggerIP),n=e.fishyDevices,Array.prototype.forEach.call(n,function(e,i){a.push(addDevice(e))});else{alert(\"Failed to load network.JSON. Suspect one of the devices is not configured properly. Look at http://[this device's ip]/network.JSON (link should appear when you click OK) to see if you can find which device is affecting the JSON data and correct it. Often this occurs when a device is compiled with the wrong 'CUSTOM_DEVICE_TYPE' setting.\");var i=\"<div class='CPdevice' id='CP-\"+window.location.hostname+\"'>\",r=window.location.href;i+=\"<a style='color:red' href = '\"+r.substring(0,r.lastIndexOf(\"/\"))+\"/network.JSON' >network.JSON</a>\",i+=\"</div>\",a.push(i)}(l=document.createElement(\"DIV\")).className=\"flex-container\",l.id=\"flex-container\",l.innerHTML=a.join(\"\"),o.insertBefore(l,t)})}window.onload=buildPage(),isCtrl&&_(\"swRefreshChck-1\").checked&&(pollTimer=setInterval(function(){pollJSON()},1e4))</script></div><div class=overlay id=myModal><a class=closebtn href=javascript:void(0) onclick=closeModal()><div class=closebtn id=closeBtn>&#10060;</div></a><div class=modal-body id=myIframeDiv><iframe class=iframeBody height=100% id=myIframe src=\"\"width=100%></iframe></div></div><script>var modal=_(\"myModal\"),closeBtn=_(\"closeBtn\");function closeModal(){modal.style.height=\"0%\",closeBtn.style.display=\"none\",_(\"myIframe\").src=\"\"}var span=document.getElementsByClassName(\"close\")[0];function openModal(e){_(\"myIframeDiv\").innerHTML=\"<iframe  id='myIframe' class='iframeBody' width=100% height=100% src='http://\"+e+\"/control'></iframe>\",modal.style.height=\"100%\",closeBtn.style.display=\"block\"}</script>";
/*===========================================================================*/

/*===========================================================================*/
//COMMON JAVASCRIPT FUNCTION FILE - Used to help all the fishyDIY webpages run
//SEE SOURCE FILE FOR CHANGES/TESTING IN fishyDIYdevices/extras/html/CommonWebFunctions.js
static const char WEBSTR_COMMON_JS[] PROGMEM = "var websock;function _(e){return document.getElementById(e)}function alertBadBrowser(){var e=!!document.documentMode,o=!e&&!!window.StyleMedia;(e||o)&&alert(\"Your browser may cause display problems. You should obtain a modern webkit browser.  While some versions of Microsoft Edge work, it can be buggy. Chrome, Firefox, Safari, and Opera all work consistently. Internet Explorer is not supported.\")}function swMstrUpd(){var e=_(\"swMstrLab\");!0===_(\"swMstrChck\").checked?e.innerHTML=\"Master Node\":e.innerHTML=\"Slave Node\"}function returnColor(){document.body.style.backgroundColor=\"var(--bg-col)\"}function processJSON(e){var o=JSON.parse(e);doStuffWithJSON(o.fishyDevices)}function blockSpecialChar(e){var o=e.keyCode;return o>64&&o<91||o>96&&o<123||8==o||16==o||95==o||32==o||o>43&&o<47||o>=48&&o<=57}function numberCharOnly(e){var o=e.keyCode;return 8==o||16==o||o>=48&&o<=57}function loadJSON(e,o,n){var t=new XMLHttpRequest;t.onreadystatechange=function(){if(t.readyState===XMLHttpRequest.DONE)if(200===t.status){if(o){var e;try{var r=t.responseText;e=JSON.parse(r)}catch(o){o instanceof SyntaxError?(console.log(o,!0),e=null):(console.log(o,!1),e=null)}o(e)}}else n&&n(t)},t.open(\"GET\",e,!0),t.send()}function showDetails(){_(\"infoDiv\").innerHTML=infoText,_(\"infoPanel\").style.display=\"block\"}function getMsg(e){var o=e.indexOf(\"MSG:\")+4,n=e.indexOf(\"~*~*\");return e.substring(o,n)}function getNodeJSONtext(e){var o=e.indexOf(\"~*~*DAT:\")+8;return e.substring(o,e.length)}function footer(e,o,n){var t,r,a;return e?(r=\"/\",a=\"<a href='http://\"+n+\"/monitorHome.php'>[Monitor Home]</a>\"):(r=\"http://\"+o+\"/\",a=\"<a href='/monitorHome.php'>[Home Monitor]</a><a href='/configureHome.html'>[Setup Home]</a><a href='configureNetwork.html'>[Setup Device Network]</a>\"),t=\"<a href='\"+r+\"'>[Controls]</a>  <a href='\"+r+\"SWupdater'>[Software Update]</a><a href='\"+r+\"WIFIupdater'>[Set WiFi]</a>\",\"(IP unset)\"!=n&&\"0.0.0.0\"!=n&&(t+=a),t}function start(){(websock=new WebSocket(\"ws://\"+window.location.hostname+\"/ws\")).onopen=function(e){console.log(\"websock open\")},websock.onclose=function(e){return console.log(\"websock close\"),alert(\"This control panel is no longer connected to the device.  Please close this window and reopen the control panel.\"),0},websock.onerror=function(e){console.log(e)},websock.onmessage=function(e){var o=e.data;o.indexOf(\"~*~*\")<0?0===o.indexOf('{\"fishyDevices\"')?processJSON(o):console.log(o):dealWithMessagePayload(o)}}";
/*===========================================================================*/

/*===========================================================================*/
//SOFTWARE UPDATE DEV FORM PARTS----
//SEE SOURCE FILE FOR CHANGES/TESTING IN fishyDIYdevices/extras/html/CommonSWUpdateTemplate.html
static const char WEBSTR_SWUPDATE_PT1[] PROGMEM = "<!doctypehtml><style>a{color:#fff}.main,body{font-family:\"Lucida Sans Unicode\",\"Lucida Grande\",sans-serif;margin:0}#loader{position:absolute;left:50%;top:50%;z-index:1;width:150px;height:150px;margin:-75px 0 0 -82px;border:16px solid #f3f3f3;border-radius:50%;border-top:16px solid #3498db;width:120px;height:120px;-webkit-animation:spin 2s linear infinite;animation:spin 2s linear infinite}#countdown{position:absolute;width:60px;height:60px;margin:-37px 0 0 -37px;left:50%;top:50%;z-index:10;font-size:50px;color:#3498db}@-webkit-keyframes spin{0%{-webkit-transform:rotate(0)}100%{-webkit-transform:rotate(360deg)}}@keyframes spin{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}</style><script>function showLoader(e){_(\"loader\").style.display=\"block\",_(\"countdown\").style.display=\"block\",_(\"formDiv\").style.display=\"none\",_(\"SWinfoDiv\").style.display=\"none\",setTimeout(\"location.href = '/SWupdateGetForm';\",1e3*e);var n,o=e;setInterval(function(){n=(n=o)<10?\"0\"+n:n,_(\"countdown\").innerHTML=n,--o<0&&(o=0,_(\"countdown\").innerHTML=\"<span style='font-size:16px;'>...Wait...</span>\")},1e3)}function _(e){return document.getElementById(e)}function uploadFile(){_(\"progressBar\").value=0,_(\"progressBar\").style.display=\"block\";var e=_(\"file1\").files[0],n=new FormData;n.append(\"file1\",e);var o=new XMLHttpRequest;o.upload.addEventListener(\"progress\",progressHandler,!1),o.addEventListener(\"load\",completeHandler,!1),o.addEventListener(\"error\",errorHandler,!1),o.addEventListener(\"abort\",abortHandler,!1),o.open(\"POST\",\"/SWupdatePostForm\"),o.send(n)}function progressHandler(e){_(\"loaded_n_total\").innerHTML=\"Uploaded \"+e.loaded+\" bytes of \"+e.total;var n=e.loaded/e.total*100;_(\"progressBar\").value=Math.round(n),_(\"status\").innerHTML=Math.round(n)+\"% uploaded...\",100==_(\"progressBar\").value&&(_(\"status\").innerHTML=\"Done. Rebooting.\",showLoader(SWDELAY))}function completeHandler(e){_(\"status\").innerHTML=e.target.responseText,_(\"progressBar\").value=0}function errorHandler(e){_(\"status\").innerHTML=\"Upload Failed\"}function abortHandler(e){_(\"status\").innerHTML=\"Upload Aborted\"}</script><body style=margin:0><div id=loader style=display:none></div><div id=countdown style=display:none>10</div><div id=SWinfoDiv>";

static const char WEBSTR_SWUPDATE_PT2[] PROGMEM = "<div id=formDiv><hr><form enctype=multipart/form-data id=upload_form method=post><input id=file1 name=file1 onchange=uploadFile() type=file><br><progress id=progressBar max=100 style=width:50%;display:none value=0></progress><h3 id=status></h3><p id=loaded_n_total></form></div>";
/*===========================================================================*/

/*

FAUXMO ESP - 

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

PROGMEM const char FAUXMO_TCP_HEADERS[] =
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n";

PROGMEM const char FAUXMO_TCP_STATE_RESPONSE[] = "["
    "{\"success\":{\"/lights/%d/state/on\":%s}},"
    "{\"success\":{\"/lights/%d/state/bri\":%d}}"   // not needed?
"]";

// Working with gen1 and gen3, ON/OFF/%, gen3 requires TCP port 80
PROGMEM const char FAUXMO_DEVICE_JSON_TEMPLATE[] = "{"
    "\"type\":\"Extended color light\","
    "\"name\":\"%s\","
    "\"uniqueid\":\"%s-%d\","
    "\"modelid\":\"LCT007\","
    "\"state\":{"
        "\"on\":%s,\"bri\":%d,\"xy\":[0,0],\"reachable\": true"
    "},"
    "\"capabilities\":{"
        "\"certified\":false,"
        "\"streaming\":{\"renderer\":true,\"proxy\":false}"
    "},"
    "\"swversion\":\"5.105.0.21169\""
"}";

PROGMEM const char FAUXMO_DESCRIPTION_TEMPLATE[] =
"<?xml version=\"1.0\" ?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion><major>1</major><minor>0</minor></specVersion>"
    "<URLBase>http://%d.%d.%d.%d:%d/</URLBase>"
    "<device>"
        "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
        "<friendlyName>Philips hue (%d.%d.%d.%d:%d)</friendlyName>"
        "<manufacturer>Royal Philips Electronics</manufacturer>"
        "<manufacturerURL>http://www.philips.com</manufacturerURL>"
        "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
        "<modelName>Philips hue bridge 2012</modelName>"
        "<modelNumber>929000226503</modelNumber>"
        "<modelURL>http://www.meethue.com</modelURL>"
        "<serialNumber>%s</serialNumber>"
        "<UDN>uuid:2f402f80-da50-11e1-9b23-%s</UDN>"
        "<presentationURL>index.html</presentationURL>"
    "</device>"
"</root>";

PROGMEM const char FAUXMO_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=100\r\n" // SSDP_INTERVAL
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n" // _modelName, _modelNumber
    "hue-bridgeid: %s\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"  // _deviceType
    "USN: uuid:2f402f80-da50-11e1-9b23-%s::upnp:rootdevice\r\n" // _uuid::_deviceType
    "\r\n";

#endif