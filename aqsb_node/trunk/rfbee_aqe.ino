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
#include <SoftwareSerial.h>

EggBus eggBus;

//#define DHT22_NO_FLOAT //prevent floatingpoint-stuff
DHT22 dht(5);

//#define _GPS_NO_STATS
TinyGPS gps;

SoftwareSerial nss(6, 7);

unsigned char dtaUart[100];
unsigned char dtaUartLen = 0;


void setup(){
  RFBEE.init();
  Serial.begin(9600);
  Serial.println("ok");
  Serial.println(freeRam());
  //nss.begin(9600);
}

void loop()
{
  uint8_t   egg_bus_address;


  eggBus.init();
  unsigned long no2;
  unsigned long co;


  while(egg_bus_address = eggBus.next()){

//Serial.println(egg_bus_address, HEX);
//printAddress(eggBus.getSensorAddress()); //MAC

    uint8_t numSensors = eggBus.getNumSensors();
    for(uint8_t ii = 0; ii < numSensors; ii++){

      if (strncmp(eggBus.getSensorType(ii), "CO", 2) == 0) co = eggBus.getSensorValue(ii);
      else if(strncmp(eggBus.getSensorType(ii), "NO2", 3) == 0) no2 = eggBus.getSensorValue(ii);
    }
  }
/*
  Serial.print("no2: ");
  Serial.println(no2);
  Serial.print("co: ");
  Serial.println(co);
*/

  long lon=gps.GPS_INVALID_ANGLE, lat = gps.GPS_INVALID_ANGLE;
  feedgps();
  gps.get_position(&lat, &lon);
/*
  Serial.print("lat: ");
  Serial.println(lat);
  Serial.print("lon: ");
  Serial.println(lon);

*/
  //DHT22_ERROR_t errorCode;
  //errorCode = 
  dht.readData();
  short int h = dht.getHumidityInt();
  short int t = dht.getTemperatureCInt();

/*
  Serial.print("hum: ");
  Serial.println(h);
  Serial.print("temp: ");
  Serial.println(t);
*/
  /*
    MSG:
   |  4 |  4 |    9    | 9       | 8      | 8      |        |
   |HUM |TEM |   LAT   | LON     | NO2    | CO     | MAC    |
   |----|----|---------|---------|--------|--------|        |
   |0000|0000|001111111|111222222|22223333|33333344|44444444|
   |0123|4567|890123456|789012345|67890123|45678901|23456789|
              
    -995  
     425
     
  */

char msg[42];
/*     
  int i = 0;
  //fill the msg
    if(i >=0 && i<4){
      //Humidity
      char hum[10];
      itoa(h, hum, 10);
      short unsigned int zeros = 4 - strlen(hum);
      short unsigned int j = 0;
      while (j + zeros < 4){
        if (zeros > 0){
          msg[i] = '0';
          zeros--;
          i++;
        }else{
          msg[i+j] = hum[j];
          j++;
        }
      }
      i = i + j - zeros;
    }

    if(i >=4 && i<8){
      //Temperature
      char temp[10];
      itoa(t, temp, 10);
      short unsigned int zeros = 4 - strlen(temp);
      short unsigned int j = 0;
      while (j + zeros < 4){
        if (zeros > 0){
          msg[i] = '0';
          zeros--;
          i++;
        }else{
          msg[i+j] = temp[j];
          j++;
        }
      }
      i = i + j-zeros;
    }
    
    if(i >=8 && i<17){
      //Latitude
      char l[10];
      ltoa(lat, l, 10);
      
      int j = 0;
      while(j < 9){
        msg[i+j] = l[j];
        j++;
      }
      i = i + j;
    }
   
    if(i >=17 && i<26){
      //Longitude
      char l[10];
      ltoa(lon, l, 10);
      int j = 0;
      while(j < 9){
        msg[i+j] = l[j];
        j++;
      }
      i = i + j;
    }
    
    if(i >=26 && i<34){
      //NO2
      char n[10];
      ltoa(no2, n, 10);
      short unsigned int zeros = 8 - strlen(n);
      short unsigned int j = 0;
      while(j + zeros < 8){
        if (zeros > 0){
          msg[i] = '0';
          zeros--;
          i++;
        }else{
          msg[i+j] = n[j];
          j++;
        }
      }
      i = i + j - zeros;
    }
    
    if(i >=34 && i<42){
      //CO
      char c[10];
      ltoa(co, c, 10);
      short unsigned int zeros = 8 - strlen(c);
      short unsigned int j = 0;
      while(j + zeros < 8){
        if (zeros > 0){
          msg[i] = '0';
          zeros--;
          i++;
        }else{
          msg[i+j]=c[j];
          j++;
        }
      }
      i = i + j - zeros;
    }
 */   
    /*
    if(i = 42) {
     msg[i] = '\0'; 
    }
    */

 sprintf(msg, "%04d%04d%09li%09li%08lu%08lu",h,t,lat,lon,no2, co);
 /*
  for (int i = 0; i < 42; i++)
    Serial.print(msg[i]);
  
  Serial.println();
 */ 
  RFBEE.sendDta(41,(unsigned char*)msg);
}

void feedgps(){
  for (unsigned long start = millis(); millis() - start < 500;) //Parse NMEA sentences for 500ms
  {
    while (Serial.available())
    {
      char c = Serial.read();
      // Serial.write(c);
      if (gps.encode(c));
      //return true;
    }
  }
  //return false;
}


void printAddress(uint8_t * address){
  for(uint8_t jj = 0; jj < 6; jj++){
    if(address[jj] < 16) Serial.print("0");
    Serial.print(address[jj], HEX);
    if(jj != 5 ) Serial.print(":");
  }
  Serial.println();
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/*********************************************************************************************************
 * END FILE
 *********************************************************************************************************/



