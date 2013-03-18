#include <EEPROM.h>
//#include <RFBeeSendRev.h> //Modified!
//#include <RFBeeCore.h>
//#include <SD.h>

#include <avr/wdt.h>

#define DTALEN 55 //Length of a message

struct message{
  char mac[13];
  float lat, lon;
  unsigned long co, no2;
  short int hum, tem;
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
  000000000011|1|111111122|222222223|3333|3333|34444444|44455555|
  012345678901|2|345678901|234567890|1234|5678|90123456|78901234|
  */
  int result;
  if (newdata){
    struct message s_msg;
    s_msg = splitMessage(c_msg);
    Serial.print(s_msg.mac);
    Serial.print("  lat: ");
    Serial.print(s_msg.lat, 5);
    Serial.print("  lon: ");
    Serial.println(s_msg.lon, 5);
  }
  
}

/*
  Split the Message.
 */
struct message splitMessage(String c_msg){
  /*
  for (int i = 0; i < msglength; i++){
   Serial.write(c_msg[i]); 
  }*/
  struct message s_msg;
  
  //Serial.println(c_msg);
  
  char buf[13];
  
  c_msg.substring(0,12).toCharArray(buf, 13);
  memcpy(s_msg.mac, buf, strlen(buf)+1);
  //Serial.print(s_msg.mac);
  //Serial.print("-");

  c_msg.substring(13,22).toCharArray(buf, 13);
  s_msg.lat = float(atol(buf))/100000;
  //Serial.print(s_msg.lat);
  //Serial.print("-");
  
  c_msg.substring(22,31).toCharArray(buf, 13);
  s_msg.lon = float(atol(buf))/100000;
  //Serial.print(s_msg.lon);
  //Serial.print("-");
  
  c_msg.substring(31,35).toCharArray(buf, 13);
  s_msg.hum = atoi(buf);
  //Serial.print(s_msg.hum);
  //Serial.print("-");
  
  c_msg.substring(35,39).toCharArray(buf, 13);
  s_msg.tem = atoi(buf);
  //Serial.print(s_msg.tem);
  //Serial.print("-");
  
  c_msg.substring(39,47).toCharArray(buf, 13);
  s_msg.no2 = atol(buf);
  //Serial.print(s_msg.no2);
  //Serial.print("-");
  
  c_msg.substring(47,55).toCharArray(buf, 13);
  s_msg.co = atol(buf);
  //Serial.println(s_msg.co);
      
  return s_msg; //Everything  OK
}
