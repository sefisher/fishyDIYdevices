//=========================================================================================
//CUSTOM GLOBALS - For 2-state-actuator 
//=========================================================================================
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