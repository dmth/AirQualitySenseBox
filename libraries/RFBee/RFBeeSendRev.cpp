/*
  RFBeeSendRev.cpp
  2012 Copyright (c) Seeed Technology Inc.  All right reserved.

  Author:Loovee

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//#include <string.h>
#include "RFBeeSendRev.h"
#include "RFBeeGlobals.h"

#define GDO0 2      // used for polling the RF received data

/*********************************************************************************************************
** Function name: init
** Descriptions:  Initialization
*********************************************************************************************************/
void rfbeeSendRev::init(unsigned char myId, unsigned char ac)
{
    if (Config.initialized() != OK)
    {

		//Serial.begin(38400);
		//Serial.println("Initializing config");
	
        Config.reset();
	Config.set(CONFIG_MY_ADDR, myId);
	if (ac >=0 && ac <= 2) Config.set(CONFIG_ADDR_CHECK, ac);
    }
    else
    {
	Config.set(CONFIG_MY_ADDR, myId);
	if (ac >=0 && ac <= 2) Config.set(CONFIG_ADDR_CHECK, ac);
      //Serial.begin(38400);
    }
    CCx.PowerOnStartUp();
    setCCxConfig();
    pinMode(GDO0,INPUT);// used for polling the RF received data

}

/*********************************************************************************************************
** Function name: isDta
** Descriptions:  is rfbee get data
*********************************************************************************************************/
bool rfbeeSendRev::isDta()
{
    if(digitalRead(2) == HIGH)
		return 1;
	return 0;
}

/*********************************************************************************************************
** Function name: sendDta
** Descriptions:  send data, len: length of buff, dta: data buf
*********************************************************************************************************/
void rfbeeSendRev::sendDta(int len, unsigned char *dta, unsigned char destination)
{
    if (destination == 0) destination = Config.get(CONFIG_DEST_ADDR);
    transmitData(dta,len,Config.get(CONFIG_MY_ADDR), destination); 
}

rfbeeSendRev RFBEE;

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
