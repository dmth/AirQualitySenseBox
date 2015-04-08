#if BOARDTYPE==2
  void setupNetwork(){
    Serial << F("Starting Network.\n");
    //Begin Ethernet, try DHCP first
    if (Ethernet.begin(mac) == 0) {
      Serial << F(">>!!>> Failed to configure Ethernet using DHCP\n");
      // If not possible, configure static ip
      Ethernet.begin(mac, ip, gateway, subnet); 
      Serial << F("Using Hardcoded IP\n");
      // while(true); //old code instead of static IP: do nothing:
    }
    else{
      Serial << F(">>>> My IP address: ");
      for (byte thisByte = 0; thisByte < 4; thisByte++) {
	// print the value of each byte of the IP address:
	Serial.print(Ethernet.localIP()[thisByte], DEC);
	Serial << F(".");
      }
      Serial.println();
    }
  }
#endif


void toggleLed(boolean state, int ledpin){
  if (state){
    digitalWrite(ledpin, LOW);
    state = !state;
  }
  else{
    digitalWrite(ledpin, HIGH);
    state = !state;
  }
}

void LedOn(boolean state, int ledpin){
  if (!state){
    digitalWrite(ledpin, LOW);
  }
  else{
    digitalWrite(ledpin, HIGH);
  }
}

void initLED(int ledpin){
  pinMode(ledpin, OUTPUT);
}


// Write MSG to the Device's logfile
int writeFile(char hash[9], String msg){
  if (!SD.exists(hash)){
    SD.mkdir(hash);
  }
  char path[18];
  strcpy(path, hash);
  strcat(path, "/data.log");
   Serial.print("File: ");
   Serial.print(path);

  file = SD.open(path, FILE_WRITE);

  if (file){
    file.println(msg);
    file.close();
    Serial.println("... written");
    //Serial.println(msg);
    return 0;
  }
  return -1;
}

/*
  Split the Message into a structure
     Message:
     12          ||    9    ||9        ||4   ||  4 || 8      || 8      ||3  ||1 = 58
     MAC         ||  LAT    ||LON      ||HUM ||TEM || NO2    || CO     ||BAT||!
     ------------||---------||---------||----||----||--------||--------||---||!
     000000000011||111111112||222222222||3333||3333||33444444||44445555||555||5
     012345678901||234567890||123456789||0123||4567||89012345||67890123||456||7

     0004A39FEFFC  999999999  999999999  0860  0899  0003040800000317000!

     0003A39FABD3  999999999  999999999  0379  0237  00019350  00058835  000  !
     0004A303D52F  999999999  999999999  0335  0255  00015787  00112697  000  !
*/
struct message splitMessage(String c_msg){

  struct message s_msg;

  char buf[13];

  c_msg.substring(0,12).toCharArray(buf, 13);
  memcpy(s_msg.mac, buf, strlen(buf)+1);

  c_msg.substring(12,21).toCharArray(buf, 13);
  s_msg.lat = (strcmp(buf,"999999999"))? atol(buf) : 0; //Long

  c_msg.substring(21,30).toCharArray(buf, 13);
  s_msg.lon = (strcmp(buf,"999999999"))? atol(buf) : 0; //long

  c_msg.substring(30,34).toCharArray(buf, 13);
  s_msg.hum = atoi(buf);

  c_msg.substring(34,38).toCharArray(buf, 13);
  s_msg.tem = atoi(buf) - 1000; //substract 1000 this was added to prevent overflows!

  c_msg.substring(38,46).toCharArray(buf, 13);
  s_msg.no2 = getSensorValue(0,atol(buf)); //Interpolation!

  c_msg.substring(46,54).toCharArray(buf, 13);
  s_msg.co = getSensorValue(1,atol(buf)); //Interpolation
  
  c_msg.substring(54,57).toCharArray(buf, 13);
  s_msg.bat = atoi(buf);
  
  return s_msg; //Everything  OK
}

//merge MAC and NAME to an ID (for instance MAC_-_temp)
void buildID(char id[20], char mac[13], char* name, int len){
 for(int i = 0; i<20; i++)id[i] = '\0';
 for(int i = 0; i<12; i++){
   id[i] =  mac[i];
 } 
 id[12] = '_';
 id[13] = '-';
 id[14] = '_';
 for(int i=15; ((i<15+len) && (i < 20)) ;i++){
  id[i] =  name[i-15];
 }
}

