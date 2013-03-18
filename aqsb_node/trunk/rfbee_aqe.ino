#include <EEPROM.h>
#include <RFBeeSendRev.h> //Modified!
#include <RFBeeCore.h>

#include <DHT22.h>
#include <Wire.h>
#include <EggBus.h> // This library has been extended to fit the 1.4 Shields!
       //See Post: https://groups.google.com/d/msg/airqualityegg/1W1BYTyfhRU/uaRWM8m4w1oJ
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
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8secr
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
  setup_watchdog(8); //sleep for 4 Seconds
  
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
DHT22 dht(6);
//#define _GPS_NO_STATS
TinyGPS gps;
uint8_t mac[6]; //MAC Address of the AQ-Shield


#define BEEID 1 //0-254, Address of this RFBee, All Senders have 1 // 0 is Broadcast Address
#define TARGETBEE 0 // The Address of the RFBee which should receive the Message, Broadcast

//Arithmetic Mean of NUMCYCLES measurements is calculated and send away....
#define NUMCYCLES 1 //15 Cycles followed by a 4 Second break ~> 60seconds
byte cycles = 0;

unsigned long no2 = 0;
unsigned long co = 0;
int t = 0;
int h = 0;


void setup(){
  //sleep
  cbi( SMCR,SE );      // sleep enable, power down mode
  cbi( SMCR,SM0 );     // power down mode
  sbi( SMCR,SM1 );     // power down mode
  cbi( SMCR,SM2 );     // power down mode
 
  RFBEE.init(BEEID, 1); //Modified! This ID, AdressCheck 1=true 2=True+Broadcast
  Serial.begin(9600);
  //Provide power... 
  //see:  http://www.seeedstudio.com/wiki/Grove_-_XBee_Carrier#Usage
  //pinMode(16, OUTPUT);
  //digitalWrite(16, LOW);
  pinMode(17, OUTPUT); // GPS
  digitalWrite(17, LOW);
  
  //read DHT and delay two seconds.... to prevent DHT_ERROR_TOOQUICK
  dht.readData();
  delay(2000);
}

void loop()
{
  wdt_enable(WDTO_4S); //enable Watchdog. The System has 4 Seconds to complete the loop. If this can't be achieved, the device restarts....
  if (f_wdt == 1) f_wdt = 0; 
   // do your job... 

  char msg[59]; //The message that is send away...
 
  long lon=TinyGPS::GPS_INVALID_ANGLE, lat = TinyGPS::GPS_INVALID_ANGLE;

  unsigned long fixage;
  feedgps(200); //param is the time in ms how long the serial port shoul be read... 
  gps.get_position(&lat, &lon, &fixage);
  if (fixage == TinyGPS::GPS_INVALID_AGE){
    lon = TinyGPS::GPS_INVALID_ANGLE;
    lat = TinyGPS::GPS_INVALID_ANGLE;
  }
  
  eggBus.init();
  
  while(eggBus.next()){
    Serial.println("n");
    if (mac[1]+mac[2]+mac[3]+mac[4]+mac[5]+mac[0] == 0) addressToArray(mac, eggBus.getSensorAddress());

    uint8_t numSensors = eggBus.getNumSensors();
    
    for(uint8_t ii = 0; ii < numSensors; ii++){

      if (strncmp(eggBus.getSensorType(ii), "CO", 2) == 0) co += eggBus.getSensorValueOneDotFour(ii)/1000; // difference in R0 calibration 750(1.4FW) vs 750000(1.5 FW)
      else if(strncmp(eggBus.getSensorType(ii), "NO2", 3) == 0) no2 += eggBus.getSensorValueOneDotFour(ii);
    }
  }
  
  dht.readData();
  
  if (dht.getHumidityInt() != -995)
    h += dht.getHumidityInt();
  
  if (dht.getTemperatureCInt() != -995)
    t += dht.getTemperatureCInt();

  byte bat = 000;

  if (cycles++ == NUMCYCLES){
    t = t / cycles;
    h = h / cycles;
    no2 = no2 / cycles;
    co = co / cycles;
    
    
    char ts = '1';
    if (t < 0){
      ts = '0';
      t = 1000-abs(t); 
    }
    //What happens here? 
      //If the temperature is < 0 sprintf will "overflow" this is problematic as the overflow value (maximum int) is longer than our char-space in the message 
      //to make this more easy every temperature abov 0 degree will be larger than 1000, every temperature below 0 will be smaller than 1000
      // 3.6 DEG C will be presented as : 1036
      // -5,8 DEG C will be presented as : 0942
      // the receiver just hast to substract 1000 from the received msg to see the correct value.

    /*
     Message:
     12          ||    9    ||9        ||4   ||  4 || 8      || 8      |3  |1 = 58
     MAC         ||  LAT    ||LON      ||HUM ||TEM || NO2    || CO     |BAT|!
     ------------||---------||---------||----||----||--------||--------|---|!
     */
    sprintf(msg, "%02X%02X%02X%02X%02X%02X%09li%09li%04d%c%03d%08lu%08lu%03i!",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],lat,lon,h,ts,t,no2,co,bat);

    
    RFBEE.sendDta(58,(unsigned char*)msg, TARGETBEE); //61 Bytes is maximal length. See datasheet of CC1101 sec. 15.3 
    //Reset
    cycles = 0;
    t = 0;
    h = 0;
    no2 = 0;
    co = 0;
  } 
  //Serial.println(msg);
  //delay(200);
  
  wdt_disable(); //disable watchdog....

  system_sleep(); // when we wake up, we’ll return to the top of the loop

}

void feedgps(unsigned int i){
  for (unsigned long start = millis(); millis() - start < i;) //Parse NMEA sentences for i ms
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read()));
    }
  }
}

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



