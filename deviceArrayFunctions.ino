//-------------------------------------------------------------------------
//-------deviceArray funtions---------------------------
//-------------------------------------------------------------------------

//return a fishyDevice with this devices status in it.
//used primarily to add device data to array of device data Stored
//for webserver
fishyDevice makeMyFishyDevice()
{
	fishyDevice holder;
	//fill with current data
	holder.dead = false;
	holder.ip = WiFi.localIP();
	if (EEPROMdata.motorPosAtCCWset && EEPROMdata.motorPosAtCWset)
	{
		holder.isCalibrated = true;
	}
	else
	{
		holder.isCalibrated = false;
	}
	holder.isMaster = EEPROMdata.master;
	//if idle then use EEPROMdata, if moving then use real current position
	if((deviceTrueState == opened) || (deviceTrueState == closed) || (deviceTrueState == man_idle) || (deviceTrueState == unknown)){
		holder.motorPos = EEPROMdata.motorPos;
	}else{
		holder.motorPos = stepper1.currentPosition();
	}
	holder.motorPosAtCCW = EEPROMdata.motorPosAtCCW;
	holder.motorPosAtCW = EEPROMdata.motorPosAtCW;
	holder.motorPosAtFullCCW = EEPROMdata.motorPosAtFullCCW;
	holder.motorPosAtFullCW = EEPROMdata.motorPosAtFullCW;
	holder.name = String(EEPROMdata.namestr);
	holder.port = UDP_LOCAL_PORT;
	holder.openIsCCW = EEPROMdata.openIsCCW;
	holder.devType = String(EEPROMdata.typestr);
	holder.group = String(EEPROMdata.groupstr);
	holder.note = String(EEPROMdata.note);
	holder.swVer = String(EEPROMdata.swVer);
	holder.initStamp = String(EEPROMdata.initstr);

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
	
}

//store an updated device in the deviceArray at index
void updateNode(int index, fishyDevice updatedDevice)
{
	
		deviceArray[index] = updatedDevice;

}

//store a new device's data in the deviceArray
int storeNewNode(fishyDevice newDevice)
{
	
		int index = findDeadNode();
		if (index > -1)
		{
			deviceArray[index] = newDevice;
			return index;
		}
		else
			return -1;
	
}
//--------------------------------------------------------------------------
