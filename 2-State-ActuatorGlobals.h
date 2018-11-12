//=========================================================================================
//CUSTOM GLOBALS - For 2-state-actuator 
//=========================================================================================

//This says EEPROM since it is extracted from the 255 char (max) string stored in the EEPROMdata struct
//that is stored in EEPROM.  This struct is dynamic (not stored), but is encoded into the char[] then stored.
struct EEPROMdeviceData{
	bool swapLimSW = false;					//1 byte     //unique to 2-state-actuator
	bool openIsCCW = true;					//1 byte     //unique to 2-state-actuator
	bool motorPosAtCCWset = false;			//1 byte	 //unique to 2-state-actuator
	bool motorPosAtCWset = false;			//1 byte     //unique to 2-state-actuator
	int motorPos = 0; 						//4 bytes    //unique to 2-state-actuator
	int range = 0; 							//4 bytes    //unique to 2-state-actuator
}EEPROMdeviceData;

int targetPos = -1; //meaning no target
enum trueState //enum used to define stages of both static and transient status - tracked in the device only
{
	opening,
	opened,
	closing,
	closed,
	man_idle,
	unknown
}; 
static const char *trueState_String[] = {"opening", "opened", "closing", "closed", "man_idle", "unknown"};
enum calStages // enum used to define steps as a device is sequenced through stages of an auto-calibration
{
	doneCal,
	openingCal,
	closingCal
};
static const char *calStages_String[] = {"doneCal", "openingCal", "closingCal"};
calStages deviceCalStage = doneCal; //used to sequence the device through stages of auto-cal
trueState deviceTrueState = unknown; //used to track motor and gear actual (not ordered) state

//==============================================================

//Stepper Motor Full Swing Settings----------------------------------------------
// This sets the full stroke first "guess" for the range of motion between physical
// limit switches.  For the 90degree swing of the vent damper the 28BYJ-48 took about
// 3250 steps.  Setting it a little larger to ensure the limit switch is reached on
// boot up when it calibrates.  Future motion will be software limited.  This is defined
// to prevent a broken switch from running the motor continuously and causing damage.
#define FULL_SWING 10000
#define MAX_SPEED 800
#define START_SPEED 200
#define ACCELERATION 200

//Define manual step increment and speed
#define MANSTEP 16
#define MANSPEED 500

//Pin and Comm Rate Definitions - These are for nodeMCU-------------------------
// Motor pin definitions
#define motorPin1 5 // D1=GPIO5 for IN1 on the ULN2003 driver 1
#define motorPin2 4 // D2=GPIO4 for IN2 on the ULN2003 driver 1
#define motorPin3 0 // D3=GPIO0 for IN3 on the ULN2003 driver 1
#define motorPin4 2 // D4=GPIO2 for IN4 on the ULN2003 driver 1

// Switch pin definitions
#define SWpinLimitCW 14  // D5=GPIO14 for full close (CW) limit switch
#define SWpinLimitCCW 12 // D6=GPIO12 for full open (CCW) limit switch

//------------------------------------------------------------------

#define HALFSTEP 8

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);
