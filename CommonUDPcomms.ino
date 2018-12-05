//process UDP packets
void UDPprocessPacket()
{
	//USED FOR UDP COMMS
	char packetBuffer[MAXCMDSZ+56+MAXNAMELEN]; //buffer to hold incoming packet (MAXCMDSZ with potential to have MASTER_COMMAND_DATA and an IP added.)

	// if there's data available, read a packet
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		IPAddress remoteIp = Udp.remoteIP();
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Received packet of size ");
			Serial.println(packetSize);
			Serial.print("[UDPprocessPacket] From ");
			Serial.print(remoteIp);
			Serial.print(", port ");
			Serial.println(Udp.remotePort());
		}
		// read the packet into packetBufffer
		int len = Udp.read(packetBuffer, MAXCMDSZ+56+MAXNAMELEN);
		if (len > 0)
		{
			packetBuffer[len] = 0;
		}
		if (DEBUG_UDP_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Processing:");
			Serial.println(packetBuffer);
		}
		//look for messages from other devices to copy commands sent for use in making group commands
		if(strncmp(packetBuffer,"MASTER_COMMAND_DATA:",20)==0){
			if(DEBUG_MESSAGES){Serial.print("[UDPprocessPacket] Master copied this: ");Serial.println(packetBuffer);}
			recordCommand(packetBuffer);
		}else{
			executeCommands(packetBuffer, remoteIp);
		}
	}
}

//if master announce every minute and cull nodes if silent for 10 min;
//if not master annouce every 8 to avoid being culled
void UDPkeepAliveAndCull()
{
	//announce the master every 2 minutes
	if (EEPROMdata.master)
	{
		static unsigned long lastMstr = millis();
		if (millis() - lastMstr > 120000)
		{
			lastMstr = millis();
			if (DEBUG_MESSAGES){ Serial.println("[MAIN] Announcing master"); }
			UDPannounceMaster();
		}
		static unsigned long lastNodeCulling = millis();
		if (millis() - lastNodeCulling > 600000) //cull dead nodes from the list every 10 minutes
		{
			lastNodeCulling = millis();
			if (DEBUG_MESSAGES){ Serial.println("[MAIN] Removing Dead Nodes"); }
			cullDeadNodes();
		}
		static unsigned long lastAvoidCulling = millis();
		if (millis() - lastAvoidCulling > 240000) //poll the net every 4 minutes to avoid culling nodes
		{
			lastAvoidCulling = millis();
			if (DEBUG_MESSAGES){ Serial.println("[MAIN] Broadcasting for Poll"); }
			UDPbroadcast();
		}
	}

}

//at end of setup annouce your presence
void announceReadyOnUDP(){
	//announce master if this is the mastr node
	//otherwise let the master know you're here
	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	}else{
		UDPbroadcast();
	}
}

//broadcast on subnet to see who will respond
void UDPbroadcast()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;

	//process this devices data first, storing it in the deviceArray, then poll the rest of the network
	dealWithThisNode(makeMyFishyDevice());

	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("~UDP~ANYFISHYDEV_THERE");

	Udp.endPacket();
}

void UDPannounceMaster()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;
	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("~UDP~FISHYDIYMASTER");
	Udp.endPacket();
}

//put out this devices data on the net
void UDPpollReply(IPAddress remote)
{
	fishyDevice holder; //temp
	holder = makeMyFishyDevice();
	String response = "~UDP~POLL_RESPONSE ";
	Udp.beginPacket(remote, UDP_LOCAL_PORT);
/* 
send fishyDevice data.
Note - this is parsed by UDPparsePollResponse and paralleled by getJSON; if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):{ip,name,typestr,groupstr,statusstr,inError,isMaster}
*/
	response += "{" + holder.ip.toString() + "," +
			String(holder.name) + ","  +  
			String(holder.typestr) + "," + 
			String(holder.groupstr) + "," +
			String(holder.statusstr) + "," +
			String(holder.inError ? "true" : "false") + "," +
			String(holder.isMaster ? "true" : "false") + "}";  

	Udp.write(response.c_str());
	Udp.endPacket();

	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	} //make sure they know who is in charge
}

//parses a UDP poll reply and takes action
void UDPparsePollResponse(String responseIn, IPAddress remote)
{
	if (EEPROMdata.master )
	{
/* 
parse fishyDevice data.
Note - this data set is sent by UDPparsePollResponse and getNetworkJSON; UDPparseConfigResponse may be affected as well (if data is added that needs set by configuration updates)
it is also parsed by scripts in wifi-and-webserver.ino and webresources.h if adding data elements all these may need updating.  This function sends data as follows (keep this list updated):{ip,name,typestr,groupstr,statusstr,inError,isMaster}
*/

//TODO - convert to strtok()

		String response = responseIn.substring(19); //strip off "~UDP~POLL RESPONSE"
		fishyDevice holder;
		holder.dead = false;

		//IP
		int strStrt = 1;
		int strStop = response.indexOf(",", strStrt);
		String strIP = response.substring(strStrt, strStop);
		holder.ip[3] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[2] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[1] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[0] = strIP.toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strIP: " + holder.ip.toString());
		}

		//name
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.name = response.substring(strStrt, strStop);
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strName: " + holder.name);
		}

		
		//typestr
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.typestr = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] typestr: " + holder.typestr);
		}
	
		//groupstr
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.groupstr = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] groupstr: " + holder.groupstr);
		}

		//statusstr
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.statusstr = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] statusstr: " + holder.statusstr);
		}

		//inError
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.inError = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] inError: ");
			Serial.println(holder.inError ? "true" : "false");
		}

		//isMaster
		strStrt = strStop + 1;
		strStop = response.indexOf("}", strStrt);
		holder.isMaster = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] isMaster: ");
			Serial.println(holder.isMaster ? "true" : "false");
		}

		dealWithThisNode(holder);
	}
}

void UDPreportCommandToMaster(char devName[41], IPAddress devIP, char command[MAXCMDSZ]){
	char msg[MAXCMDSZ+56+MAXNAMELEN] = ""; //40 characters of string text, MAXCMDSZ for the CMD, MAXNAMELEN for the name, and 16 for the IP 
	sprintf(msg,"MASTER_COMMAND_DATA:devName=%s&devIP=%s&cmd=%s",devName,devIP.toString().c_str(),command);
	//String msg = "MASTER_COMMAND_DATA:devName=" + String(devName) + "&devIP=" + devIP.toString() + "&cmd=" + String(command);
	if(EEPROMdata.master){
		recordCommand(msg);
	} else{
		Udp.beginPacket(masterIP, UDP_LOCAL_PORT);
		Udp.write(msg);
		Udp.endPacket();
	}
}