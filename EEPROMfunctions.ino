void storeDataToEEPROM(){
	uint addr = 0;
	EEPROM.begin(EEPROMsz);
	// replace values in EEPROM
	EEPROM.put(addr, EEPROMdata);
	EEPROM.commit();
}

void retrieveDataFromEEPROM(){
	uint addr = 0;
	EEPROM.begin(EEPROMsz);
	// load EEPROM data into RAM, see it
	EEPROM.get(addr, EEPROMdata);
}