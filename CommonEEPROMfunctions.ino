void storeDataToEEPROM(){
	uint addr = 0;
	encodeDeviceCustomData(); //get the device custom data and put it into the EEPROM struct
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
	extractDeviceCustomData(); //get the device custom data from the EEPROM struct and move it to the EEPROMdeviceData struct
}