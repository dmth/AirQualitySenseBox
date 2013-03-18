#include <EEPROM.h>
#include <RFBeeSendRev.h> //Modified!
#include <RFBeeCore.h>

#include <DHT22.h>
#include <Wire.h>
#include <EggBus.h>
#include <TinyGPS.h>

//sleeping
#include <avr/sleep.h> 
#include <avr/wdt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void (*softReset) (void) = 0; //declare reset function @ address 0
volatile boolean f_wdt=1;

// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

//****************************************************************  
// set system into sleep state
// system wakes up when watchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);  // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                 // System sleeps here
  sleep_disable();              // System continues execution here when watchdog timed out (Or any other interrupt is called, i guess)
  sbi(ADCSRA,ADEN);             // switch Analog to Digitalconverter ON
  wdt_disable();//disable watchdog
}

EggBus eggBus;

//#define DHT22_NO_FLOAT //prevent floatingpoint-stuff
DHT22 dht(5);

//#define _GPS_NO_STATS
TinyGPS gps;


uint8_t mac[6]; //MAC Address of the AQ-Shield

char msg[56]; //The message that is send away...
unsigned long counter = 0;
char ctr[11]; //This char array can hold the counter...

#define BEEID 16 //0-255

//#define DEBUG

void setup(){
  //sleep
  cbi( SMCR,SE );      // sleep enable, power down mode
  cbi( SMCR,SM0 );     // power down mode
  sbi( SMCR,SM1 );     // power down mode
  cbi( SMCR,SM2 );     // power down mode
 
  RFBEE.init(BEEID); //Modified!
  Serial.begin(9600);
  #ifdef DEBUG
    Serial.println("ok");
  #endif

  //Provide power... 
  //see:  http://www.seeedstudio.com/wiki/Grove_-_XBee_Carrier#Usage
  //pinMode(16, OUTPUT);
  //digitalWrite(16, LOW);
  pinMode(17, OUTPUT); // GPS
  digitalWrite(17, LOW);

}

void loop()
{
  
  wdt_enable(WDTO_4S); //enable Watchdog. The System has 4 Seconds to complete the loop. If this can't be achieved, the device restarts....
  if (f_wdt == 1) f_wdt = 0; 
 
  // do your job... 
  #ifdef DEBUG
    Serial.print("L.S: #");
  #endif
  
  //COUNTER
  counter++;
  //char ctr[11];
  sprintf(ctr, "%010lu", counter);
  RFBEE.sendDta(10,(unsigned char*)ctr);
  #ifdef DEBUG 
    Serial.print(counter);
    Serial.print(" ms: ");
    Serial.println(millis());
    delay(100); //leave some time to recive data....
  #endif
  
  #ifdef DEBUG 
    Serial.println("GPS");
    delay(100); //leave some time to recive data....
  #endif
  
  long lon=gps.GPS_INVALID_ANGLE, lat = gps.GPS_INVALID_ANGLE;
  feedgps(100); //param is the time in ms how long the serial port shoul be read... 
  gps.get_position(&lat, &lon);
  
  #ifdef DEBUG 
    Serial.println("EB.I");
    delay(100); //leave some time to recive data....
  #endif
  
  unsigned long no2;
  unsigned long co;
  
  eggBus.init();

  #ifdef DEBUG
    Serial.println("EB.N");
  #endif
  
  while(eggBus.next()){

    #ifdef DEBUG
      Serial.println("EB.W");
    #endif
    
    if (mac[1]+mac[2]+mac[3]+mac[4]+mac[5]+mac[0] == 0) addressToArray(mac, eggBus.getSensorAddress());

    uint8_t numSensors = eggBus.getNumSensors();
    
    for(uint8_t ii = 0; ii < numSensors; ii++){
      #ifdef DEBUG
        Serial.println("EB.RS");
      #endif
      
      if (strncmp(eggBus.getSensorType(ii), "CO", 2) == 0) co = eggBus.getSensorValue(ii);
      else if(strncmp(eggBus.getSensorType(ii), "NO2", 3) == 0) no2 = eggBus.getSensorValue(ii);
    }
  }
  
   /* 
   mac[0] = 00;
   mac[1] = 11;
   mac[2] = 22;
   mac[3] = 33;
   mac[4] = 44;
   mac[5] = 55;
   co = 12345678;
   no2 = 87654321;
*/
  #ifdef DEBUG
    Serial.println("DHT.read");
    delay(100); //leave some time to recive data....
  #endif
  
  dht.readData();
  short int h = dht.getHumidityInt();
  short int t = dht.getTemperatureCInt();

  /*
   Message:
   12          |1|    9    | 9       |4   |  4 | 8      | 8      |
   MAC         |:|  LAT    | LON     |HUM |TEM | NO2    | CO     |
   ------------|:|---------|---------|----|----|--------|--------|
   */

  //char msg[56];
  //Serial.println("BuildMsg");
  sprintf(msg, "%02X%02X%02X%02X%02X%02X:%09li%09li%04d%04d%08lu%08lu",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],lat,lon,h,t,no2, co);

  #ifdef DEBUG
    for (int i = 0; i < 55; i++)
      Serial.write(msg[i]);

  Serial.println();
  #endif
  
  RFBEE.sendDta(55,(unsigned char*)msg);
  
  #ifdef DEBUG
    Serial.print("L.E: ");
    Serial.println(freeRam());
    Serial.println();
    delay(100); //Serial has to receive data BEFORE systm goes to sleep...
  #endif
  
  wdt_disable(); //disable watchdog....

  //Now we need a second watchdog to wake up the device from sleep...
  setup_watchdog(6);
  system_sleep(); // when we wake up, we’ll return to the top of the loop

}

void feedgps(unsigned int i){
  for (unsigned long start = millis(); millis() - start < i;) //Parse NMEA sentences for 500ms
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read()));
    }
  }
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





