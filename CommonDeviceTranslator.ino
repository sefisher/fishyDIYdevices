/*
THIS FILE PROVIDES TAKES GENERIC DEVICE FUNCTION CALLS AND MAKES SPECIFIC DEVICE FUNCTION CALLS (TRANSLATING FOR EACH DEVICE).  THE SPECIFIC FUNCTION NAMES FOR EACH DEVICE MUST BE INCLUDED BELOW. THE STANDARD FUNCTION CALLS ARE:
1)  operateDevice() - JUST RUNS EVERY LOOP FOR STATE MACHINE OPERATIONS IF NEEDED 
2)  deviceSetup() - RUN AT STARTUP TO PREPARE THE DEVICE FOLLOWING BOOT
3)  executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)) - REAL TIME RESPONSE TO DIRECT COMMANDS FROM NETWORK
4)  executeState(bool state) - RESPONSE TO VOICE COMMANDS (FAUXMO INTERFACE)
5)  UDPparseConfigResponse(String responseIn, IPAddress remote) - UPDATES CONFIG FROM NETORK COMMAND
6)  getStatusString() - RETURNS A STRING SUMMARIZING THE DEVICE STATUS FOR DISPLAY
7)  initializeDeviceCustomData() - encode *compiled* SETTINGS into the char[256] for storage in EEPROMdata for new devices
8)  extractDeviceCustomData() - extract SETTINGS from char[256] in EEPROMdata and put it into EEPROMdeviceData for use
9)  encodeDeviceCustomData() - encode *realtime* SETTINGS into the char[256] for storage in EEPROMdata for new devices
10) showEEPROMdevicePersonalityData() - debugging function to print data to serial
11) isCustomDeviceReady() - returns true if calibrated (as needed)
12) getDeviceSpecificJSON() - returns JSON formatted string with device status (for web, net, or internal use)

For each of these functions add a case statement below when adding a new device type.
Case numbers are in order devices are listed below (2-State-Actuator = 0)
*/

//i'm here - try the funnction list below - then call func[i]()
char devices[][21] = {"2-State-Actuator", "RGBLED"};
const char * WEBCTRLSTR[] = {TwoSAWEBCTRLSTR,RGBWEBCTRLSTR};
String (*getStatusStringCall[])() = {TwoSAgetStatusString, RGBgetStatusString};
void (*initializeDeviceCustomDataCall[])() = {TwoSAinitializeDeviceCustomData, RGBinitializeDeviceCustomData};
void (*extractDeviceCustomDataCall[])() = {TwoSAextractDeviceCustomData, RGBextractDeviceCustomData};
void (*encodeDeviceCustomDataCall[])() = {TwoSAencodeDeviceCustomData, RGBencodeDeviceCustomData};
void (*showEEPROMdevicePersonalityDataCall[])() = {TwoSAshowEEPROMdevicePersonalityData, RGBshowEEPROMdevicePersonalityData};
bool (*isCustomDeviceReadyCall[])() = {TwoSAisCustomDeviceReady, RGBisCustomDeviceReady};
String (*getDeviceSpecificJSONCall[])() = {TwoSAgetDeviceSpecificJSON, RGBgetDeviceSpecificJSON};
void (*operateDeviceCall[])() = {TwoSAoperateDevice, RGBoperateDevice};
void (*deviceSetupCall[])() = {TwoSAdeviceSetup, RGBdeviceSetup};
void (*executeCommandsCall[])(char inputMsg[MAXCMDSZ], IPAddress remote) = {TwoSAexecuteCommands, RGBexecuteCommands};
void (*UDPparseConfigResponseCall[])(String responseIn, IPAddress remote) = {TwoSAUDPparseConfigResponse, RGBUDPparseConfigResponse};
void (*executeStateCall[])(bool state) = {TwoSAexecuteState, RGBexecuteState};

int caseNum()
{
	int arraySize = sizeof(devices) / sizeof(devices[0]);
	for (int i = 0; i < arraySize; i++)
		if (strcmp(EEPROMdata.typestr, devices[i]) == 0)
			return i;
}
String getStatusString()
{
	return getStatusStringCall[caseNum()]();
}
void initializeDeviceCustomData()
{
	initializeDeviceCustomDataCall[caseNum()]();
}
void extractDeviceCustomData()
{
	extractDeviceCustomDataCall[caseNum()]();
}
void encodeDeviceCustomData()
{
	encodeDeviceCustomDataCall[caseNum()]();
}
void showEEPROMdevicePersonalityData()
{
	showEEPROMdevicePersonalityDataCall[caseNum()]();
}
bool isCustomDeviceReady()
{
	return isCustomDeviceReadyCall[caseNum()]();
}
String getDeviceSpecificJSON()
{
	return getDeviceSpecificJSONCall[caseNum()]();
}
void operateDevice()
{
	operateDeviceCall[caseNum()]();
}
void deviceSetup()
{
	deviceSetupCall[caseNum()]();
}
void executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
	executeCommandsCall[caseNum()](inputMsg, remote);
}
void UDPparseConfigResponse(String responseIn, IPAddress remote)
{
	UDPparseConfigResponseCall[caseNum()](responseIn, remote);
}
void executeState(bool state)
{
	executeStateCall[caseNum()](state);
}