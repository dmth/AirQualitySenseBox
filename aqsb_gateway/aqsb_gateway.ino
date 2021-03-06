#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
//Time
#include <Time.h>

// Switch through different board-setups
#if (defined (__AVR_ATmega2560__))
  #define BOARDTYPE 2 //2: IBOARD PRO
#else
  #define BOARDTYPE 1 // 1: GBOARD
#endif

//Hardware Dependenet Includes
#if BOARDTYPE==1 //GBOARD
  #include <SoftwareSerial.h>
#elif BOARDTYPE==2 //IBOARD PRO
  #include <Ethernet.h>
  #include <EthernetUdp.h>
  #include <Flash.h>
#endif //Boardtype
  

//Hardware Dependent Configuruation
#if BOARDTYPE == 1

  #define SD_CS 10 // !mega | gboard
  #define SS_PIN 10 //!mega | gboard
  #define rxPin 3
  #define txPin 2

  SoftwareSerial myserial(rxPin,txPin);
#elif BOARDTYPE == 2

  #define SD_CS 4 // mega | iboard pro
  #define SS_PIN 53 //mega | iboard pro
  
  /* "Local" MAC-Adresses Are in the range:
  x2-xx-xx-xx-xx-xx
  x6-xx-xx-xx-xx-xx
  xA-xx-xx-xx-xx-xx
  xE-xx-xx-xx-xx-xx
 
  of course we take sth starting with 52:
  52-DD-CC-BB-AA-XX
  */

  //byte mac[] = {0x52, 0xDD, 0xCC, 0xBB, 0xAA, 0x01};
  //byte mac[] = {0x52, 0xDD, 0xCC, 0xBB, 0xAA, 0x02};

  byte ip[] = {
    10, 64, 1, 20};
  byte gateway[] = {
    10,64,1,10};
  byte subnet[] = {
    255,255,255,0};


  IPAddress SNTP_server_IP(192,53,103,108); //ptbtime1
  //IPAddress SNTP_server_IP(10,64,1,10);
  const long timeZoneOffset = 0L; // set this to the offset in seconds to your local time;
  const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

  EthernetClient client;
  EthernetUDP Udp;
  unsigned int localPort = 8888;      // local port to listen for UDP packets
#endif


// Which Service should be addressed?
#define SERVICE 2 //1 cosm, 2 thingspeak, else sos

#if SERVICE == 1
  #define cosm 1
  #if BOARDTYPE == 2
    byte mac[] = {
      0x52, 0xDD, 0xCC, 0xBB, 0xAA, 0x01};
  #else

  #endif //boardtype

#elif SERVICE == 2
  #define thingspeak 1
  #if BOARDTYPE == 2
    byte mac[] = {
      0x52, 0xDD, 0xCC, 0xBB, 0xAA, 0x02};
  #else

  #endif //boardtype
      
#else //configuration !=1 | !=2

  #define sos 1
  
#endif


//Watchdog Timer to reset the device if its stuck.... does not work with all arduino bootloaders
#define WATCHDOG 0

#if WATCHDOG==1    //This does not work on all arduinos due to a reset-loop
  #include <avr/wdt.h> //watchdog to reset the device
#endif



#define RCVLED 13 //RCV LED //Use the onboard LED
boolean rcvledstate = false; 

#define ERRORLED 13 //Onboard Test LED
boolean errorledstate = false; 


#define DTALEN 58 //Length of a message


/* Structure Message:
 When a message is received, it can be splitted into this very structure.
 */
struct message{
  char mac[13];//, lat[10], lon[10];
  long lat, lon;
  //float co, no2;
  long co, no2;
  int hum, tem;
  short int bat;
};


//some prototypes to prevent "foo was not declared in this scope"
struct message splitMessage(String c_msg);
int writeFile(char hash[9], String msg);
float getTableXScaler(uint8_t sensorIndex);
float getTableYScaler(uint8_t sensorIndex);
float getIndependentScaler(uint8_t sensorIndex);
bool getTableRow(uint8_t sensorIndex, uint8_t row_number, uint8_t * xval, uint8_t *yval);
void buildID(char id[20], char mac[13], char* name, int len);
void initLED(int ledpin);
void toggleLed(boolean state, int ledpin);
void LedOn(boolean state, int ledpin);

#if BOARDTYPE == 2
//Time Prototypes
String timeAsISO(time_t time);
String timeAsISO();
String timeAsString(time_t time);
String timeAsString();

  void setupNetwork();
  unsigned long getNtpTime();
  unsigned long sendNTPpacket(IPAddress& address);
#endif

#ifdef cosm
  int retrieveAPIKey(char hash[9], char* key, int len);
  int retrieveFeedId(char hash[9], long* feedid);
  int sendDataToCosm(String smsg, char* key, long feedID);
  String jsonCosm(struct message msg);
#endif

#ifdef sos

#endif

#ifdef thingspeak
  int retrieveAPIKey(char hash[9], char* key, int len);
  int retrieveFeedId(char hash[9], long* feedid);
  int sendDataToThingspeak(String smsg, char* key, long feedID);
  String msgThingspeak(struct message msg);
#endif


// CRC HASHING
#if BOARDTYPE==2
  static PROGMEM prog_uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc_update(unsigned long crc, byte data)
  {
    byte tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
  }

  unsigned long crc_string(char *s)
  {
    unsigned long crc = ~0L;
    while (*s)
      crc = crc_update(crc, *s++);
     crc = ~crc;
    return crc;
  }
#endif


//Watchdog Timer Related
#if WATCHDOG==1
  volatile boolean f_wdt=1;
  // Watchdog Interrupt Service / is executed when  watchdog timed out
  ISR(WDT_vect) {
    f_wdt=1;  // set global flag
  }
#endif

//SD-Card Stuff
File file;
boolean sderror = false;

int resetCounter = 0;

void setup(){

  Serial.begin(38400);

  // set the SS pin as an output
  // (necessary to keep the board as
  // master and not SPI slave)
  pinMode(SS_PIN, OUTPUT);
  
  
#if BOARDTYPE==1
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  //digitalWrite(txPin, HIGH);
  myserial.begin(9600); //XBEE Socket
#elif BOARDTYPE==2
  Serial3.begin(9600);
  Serial << F("Free Ram: ") << FreeRam() << F("\n");

  setupNetwork();

  Udp.begin(localPort);

  setSyncProvider(getNtpTime);
  while(timeStatus()== timeNotSet)
  {
    delay(1000);
    Serial << F(">>!!>> Retrying to connect to NTP\n");
    setSyncProvider(getNtpTime); // wait until the time is set by the sync provider
  }
  Serial.print(">>>> Time Position: ");
  Serial.println(timeAsISO());
  Serial.println();

#endif

  Serial.println("init");




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

  initLED(RCVLED);
  initLED(ERRORLED);

}

void loop(){
#if WATCHDOG==1
  wdt_enable(WDTO_4S); //enable Watchdog. The System has 4 Seconds to complete the loop. If this can't be achieved, the device restarts....
  if (f_wdt == 1) f_wdt = 0;
#endif

  boolean newdata = false; //If true, a new message has been received

  unsigned int c;
  char c_msg[DTALEN+1];

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
    delay(1);
  }
#endif

  delay(200);


  if (newdata){

#if WATCHDOG==1
    wdt_reset(); // Reset the WD Timer
#endif


    LedOn(true, RCVLED); //turn rcvled on

    struct message s_msg;
    s_msg = splitMessage(c_msg);//Split the msg!

    char hash[9];
    sprintf(hash, "%08lx", crc_string(s_msg.mac));

#if BOARDTYPE==2

    Serial.println F("--- New Dataset ---");
    if      (!strcmp(s_msg.mac, "0004A39FEFFC")) Serial.println("Unit 01");
    else if (!strcmp(s_msg.mac, "0004A303D1F4")) Serial.println("Unit 02");
    else if (!strcmp(s_msg.mac, "0004A303D52F")) Serial.println("Unit 03");
    else if (!strcmp(s_msg.mac, "0004A39FABD3")) Serial.println("Unit 04");
    else                                         Serial.println("Unit N/A");

    //Serial.println();
    //Serial.println(c_msg);
    //Serial.println();

    Serial.print("  Hash: ");
    Serial.println(hash);

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


  #ifdef cosm
    String smsg = jsonCosm(s_msg);
    char key[60];
    long feedID = 0;

    //if (!sderror) writeFile(hash, smsg);

    retrieveAPIKey(hash, key, 60);
    retrieveFeedId(hash, &feedID);

    sendDataToCosm(smsg, key, feedID);
  #endif

  #ifdef thingspeak
    String smsg = msgThingspeak(s_msg);
    char key[38];
    long feedID = 0;

    //if (!sderror) writeFile(hash, smsg);

    retrieveAPIKey(hash, key, 38);
    retrieveFeedId(hash, &feedID);

    sendDataToThingspeak(smsg, key, feedID);
  #endif

    Serial.println F("--- --- --- --- ---");
#endif

    LedOn(false, RCVLED); //turn rcvled off

  }
#if WATCHDOG==1
  wdt_disable(); //disable watchdog....
#endif
}







