#include <EEPROM.h>
#include <RFBeeSendRev.h>
#include <RFBeeCore.h>

unsigned char dtaUart[100];
unsigned char dtaUartLen = 0;
bool stringComplete      = false;        // if get data

#define MYADDRESS 2 //all receivers have ID 2, this doesn't matter anyway as we are receiving broadcasts...
void setup(){
    RFBEE.init(MYADDRESS, 2);//Modified! This ID, AdressCheck 1=true 2=True+Broadcast
    Serial.begin(38400);
    Serial.println("ok");
}

unsigned char rxData1[200];               // data len
unsigned char len1;                       // len
unsigned char srcAddress1;
unsigned char destAddress1;
char rssi1;
unsigned char lqi1;
int result1;

void loop()
{
    if(RFBEE.isDta())
    {
        result1=receiveData(rxData1, &len1, &srcAddress1, &destAddress1, (unsigned char *)&rssi1 , &lqi1);
        for(int i = 0; i< len1; i++)
        {
            Serial.write(rxData1[i]);
        }
    }
    
    if (dtaUartLen) {

        RFBEE.sendDta(dtaUartLen, dtaUart);
        dtaUartLen = 0;
    }
}

void serialEvent() {

    while (Serial.available()) 
    {
        dtaUart[dtaUartLen++] = (unsigned char)Serial.read();        
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

