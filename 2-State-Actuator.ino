//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - EXTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
//EXT functions are void {operateDevice(),deviceSetup(),executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)),executeState(bool state)}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void operateDevice()
{
	operateLimitSwitchActuator();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
void deviceSetup()
{
	pinSetup();
	motorSetup();
}

//CUSTOM DEVICE FUNCTION - EXTERNAL (SAME NAME AND PARAMETERS CALLED IN ALL DEVICES)
//THIS IS A FUNCTION FOR A 2-State Actuator
//This function takes messages from some remote address (if from another node)
//that are of maximum lenght MAXCMDSZ and determines what actions are required.
//Commands can come from other nodes via UDP messages or from the web
//VALID COMMANDS: 
// (open,close,stop,gotoXXX,{hi.hello},reset_wifi,reset,calibrate,anyfishydev_there,poll_net,poll_response,
//  fishydiymaster,config[;semi-colon-separated-parameters=values])
void executeCommands(char inputMsg[MAXCMDSZ], IPAddress remote)
{
	String cmd = String(inputMsg);
	cmd.toLowerCase();
	if (cmd.startsWith("open"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded OPEN");
		}
		updateClients("Open Received");
		executeState(true); //WiFi "true" is open (going CCW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("close"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CLOSE");
		}
		updateClients("Close Received");
		executeState(false); //WiFi "false" is close (going CW to stop for OpenisCCW=true)
	}
	else if (cmd.startsWith("stop"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded STOP");
		}
		updateClients("Stop Received");
		executeStop();
	}
	else if (cmd.startsWith("goto"))
	{ 
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded GOTO (" + cmd + ")");
		}
		updateClients("Goto (" + cmd + ") Received");
		executeGoto(cmd); 
	}
	else if (cmd.startsWith("hi")||cmd.startsWith("hello"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Hello! My IP is:");
			Serial.println(WiFi.localIP().toString());
			Serial.println("[executeCommands] Here is my personality info (ignore the [Setup] tag):");
			showEEPROMPersonalityData();
		}
	}
	else if (cmd.startsWith("reset_wifi"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET_WIFI");
		}
		updateClients("Resetting WiFi");
		AsyncWiFiManager WiFiManager(&httpServer,&dns);
		WiFiManager.resetSettings();
		resetOnNextLoop = true;
		delay(2000);
	}
	else if (cmd.startsWith("reset"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded RESET");
		}
		updateClients("Rebooting Device");
		resetOnNextLoop=true;
	}
	else if (cmd.startsWith("calibrate"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CALIBRATE");
		}

		deviceCalStage = openingCal;
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[executeCommands] Going to calibration opening stage.\n");
		}
		updateClients("Calibrating");
		executeState(true);
	}
	else if (cmd.startsWith("anyfishydev_there"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded ANYFISHYDEV_THERE");
		}
		UDPpollReply(remote);
	}
	else if (cmd.startsWith("poll_net"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_NET");
		}
		UDPbroadcast();
	}
	else if (cmd.startsWith("poll_response"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded POLL_RESPONSE");
		}
		UDPparsePollResponse(String(inputMsg), remote); //want the original case for this
	}
	else if (cmd.startsWith("fishydiymaster"))
	{
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded FISHYDIYMASTER");
		}
		masterIP = remote; //update the master IP
	}
	else if (cmd.startsWith("config"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeCommands] Commanded CONFIG");
		}
		//TODO - this is the only use of the function - may want to move and group under device specific commands.
		UDPparseConfigResponse(String(inputMsg), remote); //want the original case for this
	}
	else
	{
		Serial.printf("[executeCommands] Input: %s is not a recognized command.\n", inputMsg);
	}
}


//CUSTOM DEVICE FUNCTION - EXTERNAL (FOR ALL FAUXMO ENABLED DEVICES)
// execute a WiFi received state change
// ON equates to open
// OFF equates to close
void executeState(bool state)
{
	//reset the motor timeout counter
	motorRunTime = millis();
	EEPROMdata.deviceTimedOut = false;
	//ensure motor output is enabled
	stepper1.enableOutputs();
	//determine correct direction based on OpenisCCW and correct the state
	bool correctedState = whichWay(state);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[executeState] Going to state: %s\n", state ? "ON" : "OFF");
	}
	deviceState = state;

	if (correctedState)
	{
		targetPos=-1;
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeState] Transition to moving CCW...");
		}
		deviceTrueState = moveCCW();	
	}
	else
	{
		targetPos=-1;
		if (DEBUG_MESSAGES)
		{
			Serial.println("[executeState] Transition to moving CW...");
		}
		deviceTrueState = moveCW();
	}
}
//=====================================================================================================================


//=====================================================================================================================
//BEGIN CUSTOM DEVICE FUNCTIONS - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
//set pin modes
void pinSetup(){
	//set switch pins to use internal pull_up resistor
	pinMode(SWpinLimitCW, INPUT_PULLUP);
	pinMode(SWpinLimitCCW, INPUT_PULLUP);
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
//set up motor parameters
void motorSetup(){
	//stepper motor setup
	stepper1.setMaxSpeed(MAX_SPEED);
	stepper1.setAcceleration(ACCELERATION);
	stepper1.setSpeed(0);
	stepper1.setCurrentPosition(EEPROMdata.motorPos);
	if((EEPROMdata.motorPosAtCCWset+EEPROMdata.motorPosAtCWset)==2){
		//if prior HW limits set state to man_idle to prevent unknown state effects
		deviceTrueState = man_idle;
	}
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//THIS IS A FUNCTION FOR A 2-State Actuator
void operateLimitSwitchActuator(){
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		 CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		 CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		 CCWlimSensorVal = digitalRead(SWpinLimitCW);
		 CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}

	//STATE MACHINE:
	//First -- check for a calibration in progress; if so - finish calibration.
	//second - go through all the other actions based on deviceTrueState.

	switch (deviceCalStage)
	{
	case doneCal:
		//No new manual ctrl ordered or calibration - use deviceTrueState to determine actions
		//advance the stepper if opening or closing and not at a limit (or at the correct stopping position)
		switch (deviceTrueState)
		{
		case man_idle: //idle - do nothing
		case opened:   //idle - do nothing
		case closed:   //idle - do nothing
			break;
		case opening: //opening not at limit (really just moving CCW)
			deviceTrueState = moveCCW();
			updateClients("Moving CCW");
			break;
		case closing: //closing not at limit (really just moving CW)
			deviceTrueState = moveCW();
			updateClients("Moving CW");
			break;
		case unknown: //unknown state (bootup without stored HW limits)
			if (!CWlimSensorVal)
			{ //at CW limit
				EEPROMdata.range = stepper1.currentPosition();
				EEPROMdata.motorPosAtCWset = true;
				EEPROMdata.motorPosAtCCWset = false;
				//make actuator idle
				if(EEPROMdata.openIsCCW){
					idleActuator(closed); 
				}else{
					idleActuator(opened);
				}
			}
			else if (!CCWlimSensorVal)
			{ //at CCW limit
				stepper1.setCurrentPosition(0);
				EEPROMdata.motorPosAtCCWset = true;
				EEPROMdata.range = FULL_SWING;
				EEPROMdata.motorPosAtCWset = false;
					//make actuator idle
				if(EEPROMdata.openIsCCW){
					idleActuator(opened); 
				}else{
					idleActuator(closed);
				}
			}
			else
			{
				EEPROMdata.motorPosAtCWset = false;
				EEPROMdata.motorPosAtCCWset = false;
			}
			break;
		}
		break;
	case openingCal: //calibration in first stage	
		EEPROMdata.range = FULL_SWING;		
		EEPROMdata.motorPosAtCCWset = false;			
		EEPROMdata.motorPosAtCWset = false;	
		updateClients("Calibrating - CCW");
		deviceTrueState = moveCCW();
		if (deviceTrueState == opened)
		{ //Done with stage 1 go onto closing stage
			deviceCalStage = closingCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN loop cal] Found open (CCW) limit (0 by def'n). Set motorPos to 0. Going to calibration closing stage.\n");
			}
			executeState(false);
		}
		break;
	case closingCal: //calibration in second stage
		updateClients("Calibrating - CW");
		moveCW();
		if (deviceTrueState == closed)
		{ //Done with stage 2 - done!
			deviceCalStage = doneCal;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[MAIN loop cal] Found close (CW) limit (%d). Calibration complete.\n", EEPROMdata.range);
			}
		}
		break;
	}
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//Parses string command and then executes the move.
//cmd will be of form goto### (e.g., goto034)
void executeGoto(String cmd)
{
	//reset the motor timeout counter
	motorRunTime = millis();
	EEPROMdata.deviceTimedOut = false;
	//ensure motor output is enabled
	stepper1.enableOutputs();
	int newPercentOpen = whichWayGoto(cmd.substring(4).toInt()); //STRIP OFF GOTO and correct for openisCCCW
	if(newPercentOpen == 100){
		targetPos=-1;
		deviceTrueState = moveCCW();
	}else if(newPercentOpen == 0){
		targetPos=-1;
		deviceTrueState = moveCW();
	}else{ //targetting mid position
		if (DEBUG_MESSAGES)
		{
			Serial.printf("[executeGoto] Going to percent open: %d\n", newPercentOpen);
		}
		deviceTrueState = openPercentActuator(newPercentOpen);
	}
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//stops acutator where it is
void executeStop()
{
	//reset the motor timeout counter
	motorRunTime = millis();
	EEPROMdata.deviceTimedOut = false;
	if (DEBUG_MESSAGES)
	{
		Serial.println("[executeStop] Stopping actuator");
	}
	deviceTrueState = man_idle;
	idleActuator(deviceTrueState);
}

//CUSTOM DEVICE FUNCTION - INTERNAL
//function used to do a normal WiFi or calibration opening of the actuator
// - this moves at constant speed to HW limits
trueState moveCCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}
	trueState newState;
	if (CCWlimSensorVal==0)
	{
		//reached limit - disable everything and reset motor to idle
		// -set stepper pos = 0;  (by definition fullCCW is 0) and
		// -update flag indicating the HW lim sw was found
		stepper1.setCurrentPosition(0);
		EEPROMdata.motorPosAtCCWset = true;
		newState = idleActuator(opened); //make actuator idle
		if (DEBUG_MESSAGES)
		{

			Serial.printf("[moveCCW] Reached CCW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}else{ //keep going if still have distance to travel
		newState = opening;
		
		if(targetPos>-1){ 
			//if true then moving to a given position
			stepper1.run();
			//see if done or if timedout
			if((stepper1.currentPosition()==targetPos)){
				newState = idleActuator(man_idle);
			}
			if((millis()-motorRunTime)>EEPROMdata.timeOut*1000){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}else{
			if (stepper1.speed() != -MAX_SPEED)
			{
				stepper1.setSpeed(-MAX_SPEED);
			}
			stepper1.runSpeed();
			//see if timedout
			if(((millis()-motorRunTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//function used to do a normal WiFi or calibration closing of the actuator
trueState moveCW()
{
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}
	
	trueState newState;
	if (CWlimSensorVal==0)
	{
		//reached limit - disable everything and reset motor to idle
		// -set range to current stepper pos and
		// -update flag indicating the HW lim sw was found
		EEPROMdata.range = stepper1.currentPosition();
		EEPROMdata.motorPosAtCWset = true;

		newState = idleActuator(closed); //make actuator idle
		if (DEBUG_MESSAGES)
		{

			Serial.printf("[moveCW] Reached CW stop at motor position %d; CCW: %d; CW: %d\n", stepper1.currentPosition(), CCWlimSensorVal, CWlimSensorVal);
		}
	}else{ //keep going if still have distance to travel
		newState = closing;
		
		if(targetPos>-1){ 
			//if true then moving to a given position using acceleration
			stepper1.run();
			//see if done or if timedout
			if((stepper1.currentPosition()==targetPos)){
				newState = idleActuator(man_idle);
			}
			if((millis()-motorRunTime)>EEPROMdata.timeOut*1000){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}else{
			if (stepper1.speed() != MAX_SPEED)
			{
				stepper1.setSpeed(MAX_SPEED);
			}
			stepper1.runSpeed();
			//see if timedout
			if(((millis()-motorRunTime)>EEPROMdata.timeOut*1000)){
				EEPROMdata.deviceTimedOut = true;
				newState = idleActuator(man_idle);
			}
		}
	}
	return newState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this function swaps the state (makes open command go CW) if CW is defined as open by openIsCCW=false
bool whichWay(bool in){
	bool ret=in;
	if(EEPROMdata.openIsCCW==false){
		ret = !ret;
	}
	return ret;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this function changes goto commmand values to their
//complement (100-original value) if CW is defined as 
//open by openIsCCW
int whichWayGoto(int in){
	int ret=in;
	if(EEPROMdata.openIsCCW==true){
		ret = 100-ret;
	}
	return ret;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//put the actuator/stepper-motor in an idle state, store position in EEPROM, 
//and annonuce the final position to the MASTER
trueState idleActuator(trueState idleState)
{
	targetPos = -1;
	stepper1.stop();
	stepper1.setSpeed(0);
	stepper1.disableOutputs();
	deviceTrueState = idleState;
	EEPROMdata.motorPos = int(stepper1.currentPosition());
	storeDataToEEPROM();
	UDPpollReply(masterIP); //tell the Master Node the new info
	return idleState;
}
//CUSTOM DEVICE FUNCTION - INTERNAL
//this detemines which way to goto to get to a commanded position
//sets targetPos and commands open or close
//motor CCW limit = 0, motor CW limit = range, if no target set targetPos = -1
trueState openPercentActuator(int percent)
{
	trueState newState;
	int newMotorPos = (int)(EEPROMdata.range * percent / 100);
	targetPos = newMotorPos;
	
	//get CURRENT position of the switches as wired they should be 1=switch open (not at limit)
	int CCWlimSensorVal=1;
	int CWlimSensorVal=1;
	if (EEPROMdata.swapLimSW == false) { //fix in case switches are miswired (easy to get confused on linear actuator)
		CCWlimSensorVal = digitalRead(SWpinLimitCCW);
		CWlimSensorVal = digitalRead(SWpinLimitCW);
	}else{
		CCWlimSensorVal = digitalRead(SWpinLimitCW);
		CWlimSensorVal = digitalRead(SWpinLimitCCW);
	}

	if (DEBUG_MESSAGES)
	{
		Serial.printf("[openPercentActuator] Going to %d percent open. New Pos: %d\n", percent, newMotorPos);
	}
	if (newMotorPos < stepper1.currentPosition())
	{ //NEED TO MOVE CCW
		if (CCWlimSensorVal == 1)
		{ //switch is open; not at CCW limit
			newState = opening;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CCW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), newMotorPos);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(-START_SPEED);
		}
		else
		{
			newState = idleActuator(opened);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CCW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
	else
	{
	
		if (CWlimSensorVal == 1)
		{ //switch is open; not at CW limit
			newState = closing;
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Moving CW -> NEWSTATE: %s, Pos: %d, Target: %d\n", trueState_String[newState], stepper1.currentPosition(), newMotorPos);
			}
			stepper1.enableOutputs();
			stepper1.moveTo(newMotorPos);
			stepper1.setSpeed(START_SPEED);
		}
		else
		{
			EEPROMdata.range = stepper1.currentPosition();
			newState = idleActuator(closed);
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[openPercentActuator] Reached CW limit. Pos: %d\n", stepper1.currentPosition());
			}
		}
		return newState;
	}
}

//====================================================================================================================================
