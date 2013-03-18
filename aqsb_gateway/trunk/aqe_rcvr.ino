#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
//#include <aJSON.h> //added support for long
#include <avr/wdt.h>

// Switch through different board-setups
#if (defined (__AVR_ATmega2560__))
  #define BOARDTYPE 2 //2: IBOARD PRO
#else
  #define BOARDTYPE 1 // 1: GBOARD
#endif

#if BOARDTYPE==1 //GBOARD

  #define SD_CS 10 // !mega | gboard
  #define SS_PIN 10 //!mega | gboard
  
  #include <SoftwareSerial.h>
  #define rxPin 3
  #define txPin 2
  
  SoftwareSerial myserial(rxPin,txPin);
  
#elif BOARDTYPE==2 //IBOARD PRO

  #include <Ethernet.h>
  #include <Flash.h>
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x62, 0xE0 };
  byte ip[] = { 10, 64, 1, 20 };
  byte gateway[] = {10,64,1,10};
  byte subnet[] = {255,255,255,0};
  char server[] = "api.cosm.com";
  
  EthernetClient client;
  
  #define SD_CS 4 // mega | iboard pro
  #define SS_PIN 53 //mega | iboard pro
  
#endif

#define RCVLED A2 //RCV LED

#define DTALEN 58 //Length of a message

//Thats what insid a message
struct message{
  char mac[13];//, lat[10], lon[10];
  long lat, lon;
  //float co, no2;
  long co, no2;
  short int hum, tem;
  short int bat;
};


//cosm
#define API_KEY "ROL5z8eBL0em_uvk4KresZe1y5KSAKxJajJPRVFRa3hZVT0g" // your Cosm API key
#define FEED_ID 102432 // your Cosm feed ID


//some prototypes to prevent "foo was not declared in this scope"
struct message splitMessage(String c_msg);
int writeFile(struct message msg);
float getTableXScaler(uint8_t sensorIndex);
float getTableYScaler(uint8_t sensorIndex);
float getIndependentScaler(uint8_t sensorIndex);
bool getTableRow(uint8_t sensorIndex, uint8_t row_number, uint8_t * xval, uint8_t *yval);
String jsonCosm(struct message msg);
int getLength(char* c);
void sendData(struct message msg);
void buildID(char id[20], char mac[13], char* name, int len);


//SD-Card Stuff
File file;
boolean sderror = false;

void setup(){
  
  Serial.begin(38400);
  
 
  #if BOARDTYPE==1
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    //digitalWrite(txPin, HIGH);
    myserial.begin(9600); //XBEE Socket
  #elif BOARDTYPE==2
    Serial3.begin(9600);
    
    Serial << F("Starting Network.\n");
      //Begin Ethernet, try DHCP first
      if (Ethernet.begin(mac) == 0) {
        Serial << F(">>!!>> Failed to configure Ethernet using DHCP\n");
        // If not possible, configure static ip
        Ethernet.begin(mac, ip, gateway, subnet); 
        Serial << F("Using Hardcoded IP\n");
        // while(true); //old code instead of static IP: do nothing:
      }else{
        Serial << F(">>>> My IP address: ");
        for (byte thisByte = 0; thisByte < 4; thisByte++) {
          // print the value of each byte of the IP address:
          Serial.print(Ethernet.localIP()[thisByte], DEC);
          Serial << F(".");
      }
    Serial.println();
  }
  #endif
  
  Serial.println("init");

  // set the SS pin as an output
  // (necessary to keep the board as
  // master and not SPI slave)
  pinMode(SS_PIN, OUTPUT);


  if (!SD.begin(SD_CS)) {
    sderror = true;
    return;
  }
  
  #if BOARDTYPE==2
    if (sderror){
      Serial.println("SD: failed!");
    }
    else {
      Serial.println("SD: done.");
    }
  #endif

  pinMode(RCVLED, OUTPUT);
  
}

void loop(){
  
  boolean newdata = false; //If true, a new message has been received

  unsigned int c;
  char c_msg[DTALEN+1];

boolean r = false;
  
  #if BOARDTYPE==1
    while (myserial.available() && c < DTALEN){
      byte k = myserial.read();
      if (k != '\r' || k != '\n'){
        c_msg[c++]= k;
      }
      if (c == DTALEN) newdata = true; //c = 55
    }
  #elif BOARDTYPE==2
      while (Serial3.available() && c < DTALEN){
      byte k = Serial3.read();
      if (k != '\r' || k != '\n'){
        //Serial.write(k);
        c_msg[c++]= k;
      }
      if (c == DTALEN && k=='!') newdata = true; //c = 55
      r = true;
      delay(1);
    }
  #endif
   
  delay(200);


  if (newdata){
    digitalWrite(RCVLED, HIGH);
    //Serial.println(c_msg);
    
    struct message s_msg;
    s_msg = splitMessage(c_msg);//Split the msg!
    
    #if BOARDTYPE==2
      Serial.println();
      
      if      (!strcmp(s_msg.mac, "0004A39FEFFC")) Serial.println("Unit 01");
      else if (!strcmp(s_msg.mac, "0004A303D1F4")) Serial.println("Unit 02");
      else if (!strcmp(s_msg.mac, "0004A303D52F")) Serial.println("Unit 03");
      else if (!strcmp(s_msg.mac, "0004A39FABD3")) Serial.println("Unit 04");
      else                                         Serial.println("Unit N/A");
      
      Serial.print("  mac: ");
      Serial.println(s_msg.mac);
      Serial.print("  lat: ");
      Serial.println(s_msg.lat);
      Serial.print("  lon: ");
      Serial.println(s_msg.lon);
      Serial.print("  no2: ");
      Serial.println(s_msg.no2);
      Serial.print("  co: ");
      Serial.println(s_msg.co);
      Serial.print("  t: ");
      Serial.println((float)s_msg.tem/10);
      Serial.print("  h: ");
      Serial.println((float)s_msg.hum/10);
      Serial.print("  bat: ");
      Serial.println(s_msg.bat);
      Serial.println("----");
      
    #endif
    //if (!sderror) writeFile(s_msg);
   
    sendData(s_msg);
   
    //Serial.println(freeRam());
    digitalWrite(RCVLED, LOW);
  }
}


// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:

int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten,
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

String jsonCosm(struct message msg){
  String s;
  char id[20];
  char buf[10];
  
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
        buildID(id, msg.mac, "BAT", 3);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin NO2
        s += "\"current_value\" : "; 
        s += msg.no2;
        s += ",";
        buildID(id, msg.mac, "NO2", 3);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin CO
        s += "\"current_value\" : "; 
        s += msg.co;
        s += ",";
        buildID(id, msg.mac, "CO", 2);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin Temp
        s += "\"current_value\" : ";
        dtostrf((float)msg.tem/10,4,1,buf);
        s += buf; s += ",";
        buildID(id, msg.mac, "TEMP", 4);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "},";
      
      s += "{"; //begin hum
        s += "\"current_value\" : ";
        dtostrf((float)msg.hum/10,4,1,buf);
        s += buf; s += ",";
        buildID(id, msg.mac, "RH", 2);
        s += "\"id\" : ";
        s += "\""; s+= id ; s+="\"";
      s += "}";
    s += "]"; //end of datastreams array
  s += "}"; //end
  
  return s;
}

//merge mac and name to ID
void buildID(char id[20], char mac[13], char* name, int len){
 for(int i = 0; i<20; i++)id[i] = '\0';
 for(int i = 0; i<12; i++){
   id[i] =  mac[i];
 } 
 id[12] = '_';
 id[13] = '-';
 id[14] = '_';
 for(int i=15; (i<15+len & i < 20) ;i++){
  id[i] =  name[i-15];
 }
}



// this method makes a HTTP connection to the server:
void sendData(struct message msg) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    
    String smsg = jsonCosm(msg);
    //Serial.println(smsg);
    
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    Serial.print("PUT /v2/feeds/");
    client.print(FEED_ID);
    Serial.print(FEED_ID);
    client.println(".json HTTP/1.1");
    Serial.println(".json HTTP/1.1");
    client.print("Host: ");
    Serial.print("Host: ");
    client.println(server);
    Serial.println(server);
    client.print("X-ApiKey: ");
    Serial.print("X-ApiKey: ");
    client.println(API_KEY);
    Serial.println(API_KEY);
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

 
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
}

