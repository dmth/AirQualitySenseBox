//Interpolates the resitance to a ppm value.

/*
  The methods are taken from 
    https://github.com/WickedDevice/aqe_sensor_interface_shield/blob/master/src/interpolation.c
  and
    https://github.com/jmsaavedra/Air-Quality-Egg/blob/master/libraries/EggBus/EggBus.cpp
  
  they have been adapted to be run without using the eggbus library or I2C communication.
  
  Original Author:
    Victor Aprea <victor.aprea@wickeddevice.com> 
 */


#define INTERPOLATION_X_INDEX 0
#define INTERPOLATION_Y_INDEX 1
#define INTERPOLATION_TERMINATOR 0xff

// these are the conversion factors required to turn into floating point values (multiply table values by these)
const float x_scaler[2] = {0.4f, 0.003f};
const float y_scaler[2] = {1.7f, 165.0f};
const float independent_scaler[2] = {0.0001f, 0.0004f};
const uint32_t independent_scaler_inverse[2] = { 10000, 2500 };

const unsigned int egg_bus_sensor_r0[2]  = {2200, 750}; // values in ohms
/*
  The Response Table.
  get_x_or_get_y = 0 returns x value from table, get_x_or_get_y = 1 returns y value from table
  table_index: row of the table
*/
uint8_t getTableValue(uint8_t sensor_index, uint8_t table_index, uint8_t get_x_or_get_y){
    // the values MUST be provided in ascending order of x-value
    const uint8_t no2_ppb[][2] = {
            {62,117},
            {75,131},
            {101,152},
            {149,188},
            {174,204},
            {199,219},
            {223,233},
            {247,246},
            {INTERPOLATION_TERMINATOR, INTERPOLATION_TERMINATOR}
    };
    const uint8_t co_ppb[][2] = {
            {134,250},
            {168,125},
            {202,49},
            {232,12},
            {241,6},
            {INTERPOLATION_TERMINATOR, INTERPOLATION_TERMINATOR}
    };
    // sensor index 0 is the NO2 sensor
    uint8_t return_value = 0;
    if(sensor_index == 0){
        return_value = no2_ppb[table_index][get_x_or_get_y];
    }
    else{ // sensor index 1 is for CO
        return_value = co_ppb[table_index][get_x_or_get_y];
    }
    return return_value;
}


/*
returns the computed sensor value of the index as a 32-bit integer
This method does the "heavy lifting".
It interpolates/extrapolates the resistance to ppm

ORIGINAL WAS BUGGY SEE http://blog.wickeddevice.com/?p=411
fixed w/ previous_previous...
*/
float getSensorValue(uint8_t sensorIndex, uint32_t measurement){
  float slope = 0.0;
  float x_scaler = getTableXScaler(sensorIndex);
  float y_scaler = getTableYScaler(sensorIndex);
  float i_scaler = getIndependentScaler(sensorIndex);
  uint32_t measured_value = measurement / egg_bus_sensor_r0[sensorIndex];
   
  float independent_variable_value = (i_scaler * measured_value);
  uint8_t xval, yval, row = 0;
  float real_table_value_x, real_table_value_y;
  float previous_real_table_value_x = 0.0, previous_real_table_value_y = 0.0;
  float previous_previous_real_table_value_x = 0.0, previous_previous_real_table_value_y = 0.0;
   
  while(getTableRow(sensorIndex, row++, &xval, &yval)){
    real_table_value_x = x_scaler * xval;
    real_table_value_y = y_scaler * yval;
    
    // case 1: this row is an exact match, just return the table value for y
    if(real_table_value_x == independent_variable_value){
           return real_table_value_y;
    }
    
    // case 2: this is the first row and the independent variable is smaller than it
    // therefore extrapolation backward is required
    else if((row == 1) && (real_table_value_x > independent_variable_value)){
      // look up the value in row 1 to calculate the slope to extrapolate
      previous_real_table_value_x = real_table_value_x;
      previous_real_table_value_y = real_table_value_y;
      
      getTableRow(sensorIndex, row++, &xval, &yval);
      real_table_value_x = x_scaler * xval;
      real_table_value_y = y_scaler * yval;
      
      slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
      return previous_real_table_value_y - slope * (previous_real_table_value_x - independent_variable_value);
    }
    
    // case 3: the independent variable is between the current row and the previous row
    // interpolation is required  
    else if((row > 1) && (real_table_value_x > independent_variable_value) && (independent_variable_value > previous_real_table_value_x)){
      // interpolate between the two values
      slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
      
      return previous_real_table_value_y + (independent_variable_value - previous_real_table_value_x) * slope;
    }
    
    // store into the previous values for use in interpolation/extrapolation
    // store into the previous values for use in interpolation/extrapolation
    previous_previous_real_table_value_x = previous_real_table_value_x;
    previous_previous_real_table_value_y = previous_real_table_value_y; 
    
    previous_real_table_value_x = real_table_value_x;
    previous_real_table_value_y = real_table_value_y;
  }
  
  // case 4: the independent variable is greater than the largest value in the table, so we need to extrapolate forward
  // if you got through the entire table without an early return that means the independent_variable_value
  // the last values stored should be used for the slope calculation

  slope = (real_table_value_y - previous_previous_real_table_value_y) / (real_table_value_x - previous_previous_real_table_value_x);
  return real_table_value_y + slope * (independent_variable_value - real_table_value_x);
}

/*
gets the x scaler value for the requested sensor
*/
float getTableXScaler(uint8_t sensorIndex){
  return x_scaler[sensorIndex];
}

/*
gets the y scaler value for the requested sensor
*/
float getTableYScaler(uint8_t sensorIndex){
  return y_scaler[sensorIndex];
}

/*
gets the independent variable scaler value for the requested sensor
*/
float getIndependentScaler(uint8_t sensorIndex){
  return independent_scaler[sensorIndex];
}

/*
gets the requested table row (table_index) for the requested sensor
*/
//return false if xval | yval == 0xff
bool getTableRow(uint8_t sensorIndex, uint8_t row_number, uint8_t * xval, uint8_t *yval){
  *xval = getTableValue(sensorIndex, row_number, 0);
  *yval = getTableValue(sensorIndex, row_number, 1);
  return (*xval != INTERPOLATION_TERMINATOR && *yval != INTERPOLATION_TERMINATOR);
}


int writeFile(struct message msg){
  char* filename = "msgdata.txt";
  file = SD.open(filename, FILE_WRITE);

  if (file){
    file.println(jsonCosm(msg));
    file.close();
    
    Serial.println(jsonCosm(msg));
   
    return 0;
  }
  return -1;
}

/*
  Split the Message.
 */
  //Messages send by Outdoor units look like this:
  /*
     Message:
     12          ||    9    ||9        ||4   ||  4 || 8      || 8      ||3  ||1 = 58
     MAC         ||  LAT    ||LON      ||HUM ||TEM || NO2    || CO     ||BAT||!
     ------------||---------||---------||----||----||--------||--------||---||!
     000000000011||111111112||222222222||3333||3333||33444444||44445555||555||5
     012345678901||234567890||123456789||0123||4567||89012345||67890123||456||7
     
     0003A39FABD3  999999999  999999999  0379  0237  00019350  00058835  000  !
   */
   
   
   
struct message splitMessage(String c_msg){

  struct message s_msg;

  char buf[13];

  c_msg.substring(0,12).toCharArray(buf, 13);
  memcpy(s_msg.mac, buf, strlen(buf)+1);

  c_msg.substring(12,21).toCharArray(buf, 13);
  s_msg.lat = atol(buf); //Long

  c_msg.substring(21,30).toCharArray(buf, 13);
  s_msg.lon = atol(buf); //long

  c_msg.substring(30,34).toCharArray(buf, 13);
  s_msg.hum = atoi(buf);

  c_msg.substring(34,38).toCharArray(buf, 13);
  s_msg.tem = atoi(buf);

  c_msg.substring(38,46).toCharArray(buf, 13);
  s_msg.no2 = getSensorValue(0,atol(buf));

  c_msg.substring(46,54).toCharArray(buf, 13);
  s_msg.co = getSensorValue(1,atol(buf));
  
  c_msg.substring(54,57).toCharArray(buf, 13);
  s_msg.bat = atoi(buf);
  
  return s_msg; //Everything  OK
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
