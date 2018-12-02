//=========================================================================================
//CUSTOM GLOBALS - For 2-state-actuator 
//=========================================================================================

//This says EEPROM since it is extracted from the 255 char (max) string stored in the EEPROMdata struct
//that is stored in EEPROM.  This struct is dynamic (not stored), but is encoded into the char[] then stored.
struct RGBEEPROMdeviceData{
	char rgb[8] = "#000000";					
}RGBEEPROMdeviceData;

static long lastColorUpdateTime = 0; //used to slow down response time and only do EEPROM writes after color stops changing
static bool saveComplete = false;	 //flag for save completion
#define COLOR_DELAY 100				 //delay (msec) between color changes in response to input	
#define COLOR_SAVE_WAIT 2000		 //delay (msec) after color change to save final color to EEPROM
//Pin and Comm Rate Definitions - These are for nodeMCU-------------------------
// Motor pin definitions
#define LED_RED     4 // D1=GPIO5 
#define LED_GREEN   5 // D2=GPIO4
#define LED_BLUE    0 // D3=GPIO0

