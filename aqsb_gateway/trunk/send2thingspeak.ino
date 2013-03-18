#ifdef thingspeak

char thingspeakserver[] = "api.thingspeak.com";

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

String msgThingspeak(struct message msg){
  String s;
  char id[20];
  char buf[10];
  
  char delim='&';
  
  //time_t ti = now();
  
  s += "field1=";
    s += msg.bat;
  s+= delim;
  
  s += "field2=";
    s += msg.no2;
  s+= delim;
  
  s += "field3=";
    s += msg.co;
  s+= delim;
  
  s += "field4=";
    s += msg.tem;
  s+= delim;
  
  s += "field5=";
    s += msg.hum;
  s += delim;
  
  s += "lat=";
    s+= dtostrf((float)msg.lat/(float)100000,9,5,buf);
  s += delim;
  
  s += "lon=";
    s+= dtostrf((float)msg.lon/(float)100000,9,5,buf);
  
  return s;
}

// this method makes a HTTP connection to the cosm server:
// returns 1 if succesful
int sendDataToThingspeak(String smsg, char* key, long feedID) {
  // if there's a successful connection:
  if (client.connect(thingspeakserver, 80)) {
    Serial.println("connecting...");

    // send the HTTP POST request:
    client.print("POST /update");
    Serial.print("POST /update");
    //client.print(feedID);
    //Serial.print(feedID);
    client.println(".json HTTP/1.1");
    Serial.println(".json HTTP/1.1");
    client.print("Host: ");
    Serial.print("Host: ");
    client.println(thingspeakserver);
    Serial.println(thingspeakserver);
    client.println(key);
    Serial.println(key);
    client.print("Content-Length: ");
    Serial.print("Content-Length: ");
    client.println(smsg.length());
    Serial.println(smsg.length());

    
    // last pieces of the HTTP PUT request:
    client.println("Content-Type: application/x-www-form-urlencoded");
    Serial.println("Content-Type: application/x-www-form-urlencoded");
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
    
    return 0;
  }
}

#endif
