/*
  Firmware for rfBee
 see www.seeedstudio.com for details and ordering rfBee hardware.
 
 Copyright (c) 2010 Hans Klunder <hans.klunder (at) bigfoot.com>
 Author: Hans Klunder, based on the original Rfbee v1.0 firmware by Seeedstudio
 Version: July 24, 2010
 
 This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <EEPROM.h>
#include <RFBeeSendRev.h>
#include <RFBeeCore.h>

#include <DHT22.h>
#include <Wire.h>
#include <EggBus.h>
#include <TinyGPS.h>

EggBus eggBus;

//#define DHT22_NO_FLOAT //prevent floatingpoint-stuff
DHT22 dht(5);

//#define _GPS_NO_STATS
TinyGPS gps;


uint8_t mac[6];

unsigned long counter = 0;

void setup(){
  RFBEE.init();
  Serial.begin(9600);
  Serial.println("ok");
  //Serial.println(freeRam());

  //Provide power... 
  //see:  http://www.seeedstudio.com/wiki/Grove_-_XBee_Carrier#Usage
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);
}

void loop()
{
  
  eggBus.init();
  unsigned long no2;
  unsigned long co;


  while(eggBus.next()){
  Serial.println("Reading Eggbus!");
  if (mac[1]+mac[2]+mac[3]+mac[4]+mac[5]+mac[0] == 0) addressToArray(mac, eggBus.getSensorAddress());

    uint8_t numSensors = eggBus.getNumSensors();
    for(uint8_t ii = 0; ii < numSensors; ii++){
      Serial.println("Reading Sensor!");
      if (strncmp(eggBus.getSensorType(ii), "CO", 2) == 0) co = eggBus.getSensorValue(ii);
      else if(strncmp(eggBus.getSensorType(ii), "NO2", 3) == 0) no2 = eggBus.getSensorValue(ii);
    }
  }
   
  long lon=gps.GPS_INVALID_ANGLE, lat = gps.GPS_INVALID_ANGLE;
  feedgps(0); //param is the time in ms how long the serial port shoul be read... 
  gps.get_position(&lat, &lon);

  dht.readData();
  short int h = dht.getHumidityInt();
  short int t = dht.getTemperatureCInt();

  /*
   Message:
   12          |1|    9    | 9       |4   |  4 | 8      | 8      |1|3
   MAC         |:|  LAT    | LON     |HUM |TEM | NO2    | CO     |-|Ctr
   ------------|:|---------|---------|----|----|--------|--------|-|---  
  */

 char msg[60];


 sprintf(msg, "%02X%02X%02X%02X%02X%02X:%09li%09li%04d%04d%08lu%08lu-%03lu",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],lat,lon,h,t,no2, co, counter);
   
  for (int i = 0; i < 59; i++)
    Serial.print(msg[i]);
  Serial.println();
  
  
  RFBEE.sendDta(59,(unsigned char*)msg);
  Serial.println(freeRam());
  counter++;
  //Serial.print("Counter: ");Serial.println(counter);
  delay(500);
  Serial.println("Loop End");
}

void feedgps(unsigned int i){
  //for (unsigned long start = millis(); millis() - start < i;) //Parse NMEA sentences for 500ms
  //{
    while (Serial.available())
    {
      if (gps.encode(Serial.read()));
    }
  //}
}

/*
void printAddress(uint8_t * address){
  for(uint8_t jj = 0; jj < 6; jj++){
    if(address[jj] < 16) Serial.print("0");
    Serial.print(address[jj], HEX);
    if(jj != 5 ) Serial.print(":");
  }
  Serial.println();
}
*/

void addressToArray(byte mac[], uint8_t * address){
  for(uint8_t jj = 0; jj < 6; jj++){
    mac[jj] = address[jj];
  }
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/*********************************************************************************************************
 * END FILE
 *********************************************************************************************************/



