//-------------------------------------------------------------------------
//-------deviceArray funtions---------------------------
//-------------------------------------------------------------------------

//return a fishyDevice with this devices status in it.
//used primarily to add device data to array of device data Stored
//for webserver
fishyDevice makeMyFishyDevice()
{
	bool isReady = false; //used for the custom device functions to report any errors to include being uncalibrated
	
	fishyDevice holder;
		
	//fill with current data
	holder.dead = false;
	holder.timeStamp = millis();
	holder.ip = WiFi.localIP();
	
	isReady = isCustomDeviceReady();
	
	holder.isMaster = EEPROMdata.master;
	
	if(EEPROMdata.deviceTimedOut || !(isReady)){
		holder.inError = true;
	}else{
		holder.inError = false;
	}
	
	holder.name = String(EEPROMdata.namestr);
	holder.typestr = String(EEPROMdata.typestr);
	holder.groupstr = String(EEPROMdata.groupstr);
	holder.statusstr = getStatusString();
	
	showThisNode(holder);
	
	return holder;
}

//take an address for a node and figure out what to do with it
//return the index on success or -1 on fail
int dealWithThisNode(fishyDevice netDevice)
{
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		int index = findNode(netDevice.ip);
		if (index > -1)
		{
			updateNode(index, netDevice);
			if (DEBUG_MESSAGES)
			{
				Serial.print("[dealWithThisNode] updated ");
				Serial.print(netDevice.isMaster ? "[MASTER] " : "");
				Serial.println(netDevice.name + " @ " + netDevice.ip.toString());
			}
			return index;
		}
		else
		{
			if (DEBUG_MESSAGES)
			{
				Serial.print("[dealWithThisNode] added.. ");
				Serial.print(netDevice.isMaster ? "[MASTER] " : "");
				Serial.println(netDevice.name + " @ " + netDevice.ip.toString());
			}
			return storeNewNode(netDevice);
		}
	}
}

//lookup the index for an IPAddress in the deviceArray---------------------------
//returns -1 if not found. Otherwise returns the index.
//first verifies this is the EEPROMdata.master
int findNode(IPAddress lookupIP)
{
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		for (int i = 0; i < MAX_DEVICE; i++)
		{
			if (!deviceArray[i].dead)
			{
				if (deviceArray[i].ip == lookupIP)
				{
					return i;
				}
			}
		}
		return -1;
	}
}

//lookup the index for the first dead (empty) device ---------------------------
//returns -1 if not found. Otherwise returns the index.
//first verifies this is the EEPROMdata.master 
int findDeadNode()
{
	
		for (int i = 0; i < MAX_DEVICE; i++)
		{
			if (deviceArray[i].dead)
			{
				return i;
			}
		}
		return -1; //no more room - oh well
		//TODO!! - alert the user on the webpage when the number of devices is at  MAX_NODES and no more space can be found
}

//store an updated device in the deviceArray at index
void updateNode(int index, fishyDevice updatedDevice)
{
		updatedDevice.timeStamp = millis();
		deviceArray[index] = updatedDevice;
}

//store a new device's data in the deviceArray
int storeNewNode(fishyDevice newDevice)
{
	
		int index = findDeadNode();
		if (index > -1)
		{
			newDevice.timeStamp = millis();
			deviceArray[index] = newDevice;
			return index;
		}
		else
			return -1;
	
}
//--------------------------------------------------------------------------

//find nodes that have dropped of the net for more than about 10 minutes and mark them as dead
void cullDeadNodes(){
	unsigned long now = millis();
	long age;
	for (int i = 0; i < MAX_DEVICE; i++)
	{
		if (!deviceArray[i].dead)
		{
			age = (long)((now - deviceArray[i].timeStamp)/60000); //age in minutes (if negative then millis() has rolled over)
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[cullDeadNodes] Node %d ", i);
				Serial.print(deviceArray[i].isMaster ? "(MASTER) " : "");
				Serial.print(deviceArray[i].name + " @ " + deviceArray[i].ip.toString());
				Serial.printf(" was updated %d minutes ago. \n",age);
			}
			if(age > 7){ //7 minutes off the net seems long enough to assume the node is dead
				deviceArray[i].dead = true;
				if (DEBUG_MESSAGES)
				{
					Serial.printf("[cullDeadNodes] Node %d ",i);
					Serial.print(deviceArray[i].isMaster ? "(MASTER) " : "");
					Serial.print(deviceArray[i].name + " @ " + deviceArray[i].ip.toString());
					Serial.println(" is now marked dead.");	
				}
			}
		}
	}
}