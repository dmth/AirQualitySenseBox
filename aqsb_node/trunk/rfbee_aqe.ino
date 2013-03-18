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
// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);  // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();              // System continues execution here when watchdog timed out
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

EggBus eggBus;

//#define DHT22_NO_FLOAT //prevent floatingpoint-stuff
DHT22 dht(5);

//#define _GPS_NO_STATS
TinyGPS gps;


uint8_t mac[6];
char msg[56]; //The message that is send away...
unsigned long counter = 0;
char ctr[11]; //This char array can hold the counter...

#define BEEID 16 //0-255

void setup(){
  //sleep

  cbi( SMCR,SE );      // sleep enable, power down mode
  cbi( SMCR,SM0 );     // power down mode
  sbi( SMCR,SM1 );     // power down mode
  cbi( SMCR,SM2 );     // power down mode
  setup_watchdog(6);

  RFBEE.init(BEEID); //Modified!
  Serial.begin(9600);
  Serial.println("ok");

  //Provide power... 
  //see:  http://www.seeedstudio.com/wiki/Grove_-_XBee_Carrier#Usage
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);
}

void loop()
{
  //Sleep
  if (f_wdt==1) // wait for timed out watchdog
    f_wdt=0;   

  // do your job... 
  Serial.print("L.S: #");
  
  //COUNTER
  counter++;
  //char ctr[11];
  sprintf(ctr, "%010lu", counter);
  RFBEE.sendDta(10,(unsigned char*)ctr);
  Serial.print(counter);
  Serial.print(" ms: ");
  Serial.println(millis());
  delay(100); //leave some time to recive data....
  
  Serial.println("GPS");
  long lon=gps.GPS_INVALID_ANGLE, lat = gps.GPS_INVALID_ANGLE;
  feedgps(100); //param is the time in ms how long the serial port shoul be read... 
  gps.get_position(&lat, &lon);
  
  delay(100); //leave some time to recive data....

  Serial.println("EB.I");
  delay(100); //leave some time to recive data....
  eggBus.init();
  unsigned long no2;
  unsigned long co;

  Serial.println("EB.N");
  while(eggBus.next()){

    Serial.println("EB.W");
    if (mac[1]+mac[2]+mac[3]+mac[4]+mac[5]+mac[0] == 0) addressToArray(mac, eggBus.getSensorAddress());

    uint8_t numSensors = eggBus.getNumSensors();
    for(uint8_t ii = 0; ii < numSensors; ii++){
      Serial.println("EB.RS");
      if (strncmp(eggBus.getSensorType(ii), "CO", 2) == 0) co = eggBus.getSensorValue(ii);
      else if(strncmp(eggBus.getSensorType(ii), "NO2", 3) == 0) no2 = eggBus.getSensorValue(ii);
    }
  }

  Serial.println("DHT.read");
  delay(100); //leave some time to recive data....
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


  for (int i = 0; i < 55; i++)
    Serial.write(msg[i]);

  Serial.println();

  RFBEE.sendDta(55,(unsigned char*)msg);

  Serial.print("L.E: ");
  Serial.println(freeRam());
  Serial.println();
  
  
  delay(100); //Serial has to receive data BEFORE systm goes to sleep...
  system_sleep(); // when we wake up, we’ll return to the top of the loop

  //delay(1000);
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





