void storeDataToEEPROM(){
	uint addr = 0;
	encodeDeviceCustomData(); //get the device custom data and put it into the EEPROM struct
	EEPROM.begin(1024);
	// replace values in EEPROM
	EEPROM.put(addr, EEPROMdata);
	EEPROM.commit();
	EEPROM.end();
}

void retrieveDataFromEEPROM(){
	uint addr = 0;
	EEPROM.begin(1024);
	// load EEPROM data into RAM, see it
	EEPROM.get(addr, EEPROMdata);
	extractDeviceCustomData(); //get the device custom data from the EEPROM struct and move it to the EEPROMdeviceData struct
	EEPROM.end();
}

bool loadCredentials() {
  EEPROM.begin(1024);
  EEPROM.get(EEPROMsz+1, wifiConnect.ssid);
  EEPROM.get(EEPROMsz+1+sizeof(wifiConnect.ssid), wifiConnect.password);
  char ok[2+1];
  EEPROM.get(EEPROMsz+1+sizeof(wifiConnect.ssid)+sizeof(wifiConnect.password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    wifiConnect.ssid[0] = 0;
    wifiConnect.password[0] = 0;
  }
  if(strlen(wifiConnect.password)>0 && (String(ok) == String("OK"))){
    if(DEBUG_WIFI_MESSAGES){
      Serial.println("Recovered WiFi credentials:");
      Serial.print(wifiConnect.ssid); Serial.println(": ********\n");
    }
    return true;
  }else{
    return false;
  }
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(1024);
  if(DEBUG_WIFI_MESSAGES){Serial.print("[handleWifiSave] about to save: ");Serial.print(wifiConnect.ssid);Serial.print(" ");Serial.println(wifiConnect.password);}
  EEPROM.put(EEPROMsz+1, wifiConnect.ssid);
  EEPROM.put(EEPROMsz+1+sizeof(wifiConnect.ssid), wifiConnect.password);
  char ok[2+1] = "OK";
  EEPROM.put(EEPROMsz+1+sizeof(wifiConnect.ssid)+sizeof(wifiConnect.password), ok);
  EEPROM.commit();
  EEPROM.end();
  if(DEBUG_WIFI_MESSAGES){
	  Serial.println("\nNew WiFi credentials saved.");
  	  loadCredentials();
  }
}
