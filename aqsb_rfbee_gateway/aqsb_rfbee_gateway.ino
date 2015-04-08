#include <EEPROM.h>
#include <../libraries/RFBee/RFBeeSendRev.h>
#include <../libraries/RFBee/RFBeeCore.h>

#include <avr/wdt.h>
volatile boolean f_wdt=1;
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}


unsigned char dtaUart[100];
unsigned char dtaUartLen = 0;
bool stringComplete      = false;        // if get data

#define MYADDRESS 2 //all receivers have ID 2, this doesn't matter anyway as we are receiving broadcasts...
void setup(){
    RFBEE.init(MYADDRESS, 2);//Modified! This ID, AdressCheck 1=true 2=True+Broadcast
    Serial.begin(9600);
    //Serial.println("ok");
}

unsigned char rxData1[64];               // data len
unsigned char len1;                       // len
unsigned char srcAddress1;
unsigned char destAddress1;
char rssi1;
unsigned char lqi1;
int result1;

void loop()
{
  wdt_enable(WDTO_4S); //enable Watchdog. The System has 4 Seconds to complete the loop. If this can't be achieved, the device restarts....
  if (f_wdt == 1) f_wdt = 0;
  
    if(RFBEE.isDta())
    {
        result1=receiveData(rxData1, &len1, &srcAddress1, &destAddress1, (unsigned char *)&rssi1 , &lqi1);
        for(int i = 0; i< len1; i++)
        {
            if (rxData1[i] != '\r' || rxData1[i] != '\n')Serial.write(rxData1[i]);
        }
    }
  wdt_disable(); //disable watchdog....

}


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

