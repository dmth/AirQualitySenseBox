



 String timeAsISO(){
  char t[21];
  //t[21] = '\0';
  time_t tm = now();
  sprintf(t,"%04d-%02d-%02dT%02d:%02d:%02dZ",year(tm), month(tm), day(tm), hour(tm), minute(tm), second(tm));
  return t;
 }
 
  String timeAsISO(time_t time){
  char t[21];
  //t[21] = '\0';
  sprintf(t,"%04d-%02d-%02dT%02d:%02d:%02dZ",year(time), month(time), day(time), hour(time), minute(time), second(time));
  return t;
 }
 
  String timeAsString(){
    char t[15];
    //t[15] = '\0';
    time_t tm = now();
    sprintf(t,"%04d%02d%02d%02d%02d%02d",year(tm), month(tm), day(tm), hour(tm), minute(tm), second(tm));
    return t;
  }
  
  String timeAsString(time_t time){
    char t[15];
    //t[15] = '\0';
    sprintf(t,"%04d%02d%02d%02d%02d%02d",year(time), month(time), day(time), hour(time), minute(time), second(time));
    return t;
  }
  
  
  /*
    Time-Server Related Stuff
*/

//Get time from ntp-server
unsigned long getNtpTime()
{
  sendNTPpacket(SNTP_server_IP);
  delay(1000);
  if ( Udp.parsePacket() ) {
    
    Udp.read(packetBuffer,NTP_PACKET_SIZE);
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  

    // now convert NTP time into everyday time:

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventy_years = 2208988800UL + timeZoneOffset;    
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventy_years;  
 
    return epoch;
  }
  return 0;
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}
