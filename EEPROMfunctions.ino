//I'm here
//TODO - fix the storage and retrieval of motdata not working in general.
//store motor data to EEPROM following a command transition to idle
void storeMotorDataToEEPROM(){
	uint addr = 0;
	EEPROM.begin(EEPROMsz);
	// replace values in EEPROM
	EEPROM.put(addr, EEPROMdata);
	EEPROM.commit();
}
