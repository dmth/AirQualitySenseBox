#include <EEPROM.h>
//#include <RFBeeSendRev.h> //Modified!
//#include <RFBeeCore.h>
//#include <SD.h>

#define BEEID 2 //all recivers have ID 2, this doesn't matter anyway as we are receiving broadcasts...
#define DTALEN 55 //Length of a message

struct message{
  char mac[13];
  long lat, lon;
  unsigned long co, no2;
  short int humidity, temperature;
};

void setup(){
  Serial.begin(38400);
  Serial3.begin(38400); //XBEE Socket
  Serial.println("init");
  //RFBEE.init(BEEID); //Modified!
}


void loop(){
  boolean newdata = false; //If true, a new message has been received
  
  unsigned int c;
  char c_msg[DTALEN+1];
  
  while (Serial3.available() && c < DTALEN){
    char k = Serial3.read();
    c_msg[c++]= k;
    //Serial.write(k);
    if (c == DTALEN) newdata = true; //c = 55
  }

      delay(200);
  
  //Messages send by Outdoor units look like this:
  /*
  12          |1|    9    | 9       |4   |  4 | 8      | 8      |  = 55
  MAC         |:|  LAT    | LON     |HUM |TEM | NO2    | CO     |
  ------------|:|---------|---------|----|----|--------|--------|
  */
  int result;
  struct message s_msg;
  if (newdata){
    result = splitMessage(c_msg, DTALEN, s_msg);    
  }
  free(c_msg); 
}

int splitMessage(char* c_msg, int msglength, struct message s_msg){
  for (int i = 0; i < msglength; i++){
   Serial.write(c_msg[i]); 
  }
  Serial.println();

  return 0; //Everything OK
}
