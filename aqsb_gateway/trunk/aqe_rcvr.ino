#include <EEPROM.h>

#include <SPI.h>
#include <SD.h>
#define SD_CS 4 // pin 4 is the SPI select pin for the SDcard
//#define ETHER_CS 4
#define SS_PIN 53


#include <aJSON.h> //added support for long

File file;

#include <avr/wdt.h>

#define DTALEN 55 //Length of a message

struct message{
  char mac[13];//, lat[10], lon[10];
  long lat, lon;
  //float co, no2;
  long co, no2;
  short int hum, tem;
};

void setup(){
  Serial.begin(38400);
  Serial3.begin(38400); //XBEE Socket
  Serial.println("init");

  pinMode(SS_PIN, OUTPUT);	// set the SS pin as an output
  // (necessary to keep the board as
  // master and not SPI slave)
  // SS_PIN on Mega 53


  if (!SD.begin(SD_CS)) {
    Serial.println("SD: failed!");
    return;
  }
  Serial.println("SD: done.");

  // open the file. note that only one file can be ope
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
    
    Serial.println(c_msg);
    
    struct message s_msg;
    s_msg = splitMessage(c_msg);
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
    writeFile(s_msg);
  }
}

int writeFile(struct message msg){

  char* filename = "msgdata.log";

  file = SD.open(filename, FILE_WRITE);
  Serial.print("filewrite to:    ");
  Serial.println(filename);


  if (file){
    Serial.println("file!");

    char buf[12];
    aJsonObject *root;

    root = aJson.createObject();

    aJson.addStringToObject(root, "MAC",  msg.mac);
    //ltoa(msg.lat, buf, 10);
    aJson.addNumberToObject(root, "LAT",  msg.lat);
    //ltoa(msg.lon, buf, 10);
    aJson.addNumberToObject(root, "LON",  msg.lon);
    
    //ltoa(msg.no2, buf, 10);
    //aJson.addStringToObject(root, "NO2",  buf);
    aJson.addNumberToObject(root, "NO2", msg.no2);
    
    //ltoa(msg.co, buf, 10);
    //aJson.addStringToObject(root, "CO",   buf);
    aJson.addNumberToObject(root, "CO", msg.co);
    
    aJson.addNumberToObject(root, "TEMP", (float)msg.tem/10);
    aJson.addNumberToObject(root, "RH",   (float)msg.hum/10);

    char* c = aJson.print(root);
    aJson.deleteItem(root);
    
    file.write(c);
    file.write("\n");
    file.close();
    
    Serial.println(c);
    
    free(c);
    
    return 0;
  }
  return -1;
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

  c_msg.substring(13,22).toCharArray(buf, 13);
  //s_msg.lat = float(atol(buf))/100000; //float
  s_msg.lat = atol(buf); //Long
  //memcpy(s_msg.lat, buf, strlen(buf)+1); //char*

  c_msg.substring(22,31).toCharArray(buf, 13);
  //s_msg.lon = float(atol(buf))/100000; //float
  s_msg.lon = atol(buf); //long
  //memcpy(s_msg.lon, buf, strlen(buf)+1); //char*

  c_msg.substring(31,35).toCharArray(buf, 13);
  s_msg.hum = atoi(buf);

  c_msg.substring(35,39).toCharArray(buf, 13);
  s_msg.tem = atoi(buf);

  c_msg.substring(39,47).toCharArray(buf, 13);
  //s_msg.no2 = atol(buf);
  s_msg.no2 = getSensorValue(0,atol(buf));

  
  c_msg.substring(47,55).toCharArray(buf, 13);
  //s_msg.co = atol(buf);
  s_msg.co = getSensorValue(1,atol(buf));

  return s_msg; //Everything  OK
}


