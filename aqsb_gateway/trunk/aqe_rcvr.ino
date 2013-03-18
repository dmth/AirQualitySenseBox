#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <aJSON.h> //added support for long
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

//some prototypes to prevent "foo was not declared in this scope"
struct message splitMessage(String c_msg);
int writeFile(struct message msg);
float getTableXScaler(uint8_t sensorIndex);
float getTableYScaler(uint8_t sensorIndex);
float getIndependentScaler(uint8_t sensorIndex);
bool getTableRow(uint8_t sensorIndex, uint8_t row_number, uint8_t * xval, uint8_t *yval);


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
//    if (r) {
//      Serial.print("  c: ");
//      Serial.print(c);
//      Serial.print("  NDTA: ");
//      Serial.println(newdata);
//    }
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
    if (!sderror) writeFile(s_msg);
    
    //Serial.println(freeRam());
    digitalWrite(RCVLED, LOW);
  }
}

