
#ifdef cosm
#if BOARDTYPE == 2
//cosm
//#define FEED_ID 102432 // your Cosm feed ID
char cosmserver[] = "api.cosm.com";

// key is a char* , len is the length of that array
int retrieveAPIKey(char hash[9], char* key, int len){
  // Check if folder for this device exists, if not return -1
  if (!SD.exists(hash)){
    return -1;
  }
  
  char path[18];
  strcpy(path, hash);
  strcat(path, "/api.key");
   //Serial.print("File: ");
   //Serial.print(path);

  file = SD.open(path, FILE_READ);

  if (file){
    char ckey[len+1];
    int i = 0;
    while(file.available()){
      if (i<len) ckey[i++] = file.read();
      else break;
    }
    file.close();

    strcpy(key, ckey);
    //Serial.println("... read: ");
    
    return 0;
  }
  return -1;
}

int retrieveFeedId(char hash[9], long *feedID){
  // Check if folder for this device exists, if not return -1
  if (!SD.exists(hash)){
    return -1;
  }
  
  char path[18];
  strcpy(path, hash);
  strcat(path, "/coll.id");
   //Serial.print("File: ");
   //Serial.print(path);

  file = SD.open(path, FILE_READ);

  if (file){
    char ckey[10];
    int i = 0;
    while(file.available()){
      if (i<10) ckey[i++] = file.read();
      else break;
    }
    file.close();
    
    *feedID = atol(ckey);
    //Serial.println("... read: ");
    
    return 0;
  }
  return -1; 
}

String jsonCosm(struct message msg){
  String s;
  char id[20];
  char buf[10];
  
  time_t ti = now();
  
  s += "{"; //start
    s += "\"location\": {";
      s += "\"disposition\" : \"mobile\",";
      s += "\"lat\" :";
      dtostrf((float)msg.lat/(float)100000,9,5,buf);
      s += buf; s+= ",";
      s += "\"lon\" :";
      dtostrf((float)msg.lon/(float)100000,9,5,buf);
      s += buf; s+= ",";
      s += "\"domain\" : \"physical\"";
    s += "}"; //end of location
    s += ",";
    s += "\"datastreams\" : ["; //beginning datastreams array
    
      s += "{"; //Begin Battery
        s += "\"current_value\" : "; 
        s += msg.bat;
        s += ",";
        s += "\"at\" : \""; 
        s += timeAsISO(ti);
        s += "\",";
        buildID(id, msg.mac, "BAT", 3);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin NO2
        s += "\"current_value\" : "; 
        s += msg.no2;
        s += ",";
        s += "\"at\" : \""; 
        s += timeAsISO(ti);
        s += "\",";
        buildID(id, msg.mac, "NO2", 3);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin CO
        s += "\"current_value\" : "; 
        s += msg.co;
        s += ",";
        s += "\"at\" : \""; 
        s += timeAsISO(ti);
        s += "\",";
        buildID(id, msg.mac, "CO", 2);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin Temp
        s += "\"current_value\" : ";
        dtostrf((float)msg.tem/10,4,1,buf);
        s += buf; s += ",";
        s += "\"at\" : \""; 
        s += timeAsISO(ti);
        s += "\",";
        buildID(id, msg.mac, "TEMP", 4);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin hum
        s += "\"current_value\" : ";
        dtostrf((float)msg.hum/10,4,1,buf);
        s += buf; s += ",";
        s += "\"at\" : \""; 
        s += timeAsISO(ti);
        s += "\",";
        buildID(id, msg.mac, "RH", 2);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "}";
    s += "]"; //end of datastreams array
  s += "}"; //end
  
  return s;
}

// this method makes a HTTP connection to the cosm server:
// returns 1 if succesful
int sendDataToCosm(String smsg, char* key, long feedID) {
  // if there's a successful connection:
  if (client.connect(cosmserver, 80)) {
    Serial.println("connecting...");

    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    Serial.print("PUT /v2/feeds/");
    client.print(feedID);
    Serial.print(feedID);
    client.println(".json HTTP/1.1");
    Serial.println(".json HTTP/1.1");
    client.print("Host: ");
    Serial.print("Host: ");
    client.println(cosmserver);
    Serial.println(cosmserver);
    //client.print("X-ApiKey: ");
    //Serial.print("X-ApiKey: ");
    client.println(key);
    Serial.println(key);
    client.print("Content-Length: ");
    Serial.print("Content-Length: ");
    client.println(smsg.length());
    Serial.println(smsg.length());

    
    // last pieces of the HTTP PUT request:
    client.println("Content-Type: text/json");
    Serial.println("Content-Type: text/json");
    client.println("Connection: close");
    Serial.println("Connection: close");

    client.println();
    Serial.println();

    // here's the actual content of the PUT request:
    client.println(smsg);
    Serial.println(smsg);
    
    client.stop();
    
    return 1;
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    
    resetCounter++;
    
    if (resetCounter >=5 ) {
      Serial.println("restarting network");
      setupNetwork();
      resetCounter = 0;
      LedOn(true, ERRORLED);
    }
    
    return 0;
  }
}
#endif
#endif
