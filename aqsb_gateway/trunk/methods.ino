// Write MSG to the Device's logfile
int writeFile(char hash[9], String msg){
  if (!SD.exists(hash)){
    SD.mkdir(hash);
  }
  char path[18];
  strcpy(path, hash);
  strcat(path, "/data.log");
   Serial.print("File: ");
   Serial.print(path);

  file = SD.open(path, FILE_WRITE);

  if (file){
    file.println(msg);
    file.close();
    Serial.println("... written");
    //Serial.println(msg);
    return 0;
  }
  return -1;
}

/*
  Split the Message into a structure
     Message:
     12          ||    9    ||9        ||4   ||  4 || 8      || 8      ||3  ||1 = 58
     MAC         ||  LAT    ||LON      ||HUM ||TEM || NO2    || CO     ||BAT||!
     ------------||---------||---------||----||----||--------||--------||---||!
     000000000011||111111112||222222222||3333||3333||33444444||44445555||555||5
     012345678901||234567890||123456789||0123||4567||89012345||67890123||456||7
     
     0003A39FABD3  999999999  999999999  0379  0237  00019350  00058835  000  !
     0004A303D52F  999999999  999999999  0335  0255  00015787  00112697  000  !
*/
struct message splitMessage(String c_msg){

  struct message s_msg;

  char buf[13];

  c_msg.substring(0,12).toCharArray(buf, 13);
  memcpy(s_msg.mac, buf, strlen(buf)+1);

  c_msg.substring(12,21).toCharArray(buf, 13);
  s_msg.lat = (strcmp(buf,"999999999"))? atol(buf) : 0; //Long

  c_msg.substring(21,30).toCharArray(buf, 13);
  s_msg.lon = (strcmp(buf,"999999999"))? atol(buf) : 0; //long

  c_msg.substring(30,34).toCharArray(buf, 13);
  s_msg.hum = atoi(buf);

  c_msg.substring(34,38).toCharArray(buf, 13);
  s_msg.tem = atoi(buf);

  c_msg.substring(38,46).toCharArray(buf, 13);
  s_msg.no2 = getSensorValue(0,atol(buf)); //Interpolation!

  c_msg.substring(46,54).toCharArray(buf, 13);
  s_msg.co = getSensorValue(1,atol(buf)); //Interpolation
  
  c_msg.substring(54,57).toCharArray(buf, 13);
  s_msg.bat = atoi(buf);
  
  return s_msg; //Everything  OK
}

//merge MAC and NAME to an ID (for instance MAC_-_temp)
void buildID(char id[20], char mac[13], char* name, int len){
 for(int i = 0; i<20; i++)id[i] = '\0';
 for(int i = 0; i<12; i++){
   id[i] =  mac[i];
 } 
 id[12] = '_';
 id[13] = '-';
 id[14] = '_';
 for(int i=15; (i<15+len & i < 20) ;i++){
  id[i] =  name[i-15];
 }
}

