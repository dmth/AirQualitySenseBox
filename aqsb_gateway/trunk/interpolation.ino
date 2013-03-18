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

//uint8_t * get_p_x_scaler(uint8_t sensor_index){
//    return (uint8_t *) &(x_scaler[sensor_index]);
//}
//
//uint8_t * get_p_y_scaler(uint8_t sensor_index){
//    return (uint8_t *) &(y_scaler[sensor_index]);
//}
//
//uint8_t * get_p_independent_scaler(uint8_t sensor_index){
//    return (uint8_t *) &(independent_scaler[sensor_index]);
//}
//
//uint32_t get_independent_scaler_inverse(uint8_t sensor_index){
//    return independent_scaler_inverse[sensor_index];
//}

/*
returns the computed sensor value of the index as a 32-bit integer
This method does the "heavy lifting".
It interpolates/extrapolates the resistance to ppm
*/
float getSensorValue(uint8_t sensorIndex, uint32_t measurement){
  float slope = 0.0;
  float x_scaler = getTableXScaler(sensorIndex);
  float y_scaler = getTableYScaler(sensorIndex);
  float i_scaler = getIndependentScaler(sensorIndex);
  uint32_t measured_value = measurement / egg_bus_sensor_r0[sensorIndex];
  
  //Serial.print("DEBUG: SENS"); Serial.println(sensorIndex);
  //Serial.print("DEBUG: MES"); Serial.println(measurement);
  
  float independent_variable_value = (i_scaler * measured_value);
  uint8_t xval, yval, row = 0;
  float real_table_value_x, real_table_value_y;
  float previous_real_table_value_x = 0.0, previous_real_table_value_y = 0.0;
  
 // Serial.print("DEBUG: IV "); Serial.println(independent_variable_value, 4);
  
  while(getTableRow(sensorIndex, row++, &xval, &yval)){
    real_table_value_x = x_scaler * xval;
    real_table_value_y = y_scaler * yval;
    
    /*
    Serial.print("DEBUG: ROW "); Serial.println(row);
    Serial.print("DEBUG: IV "); Serial.println(independent_variable_value, 4);
    Serial.print("DEBUG: W RTV-X "); Serial.println(real_table_value_x);
    Serial.print("DEBUG: W Prev RTV-X "); Serial.println(previous_real_table_value_x);
    Serial.print("DEBUG: W RTV-Y "); Serial.println(real_table_value_y);
    */
    
    // Serial.print("DEBUG: C3 Bool: ");Serial.println((real_table_value_x > independent_variable_value) && (independent_variable_value > previous_real_table_value_x));
    
    // case 1: this row is an exact match, just return the table value for y
    if(real_table_value_x == independent_variable_value){
      if (sensorIndex == 1) { Serial.print("  DEBUG: C1 RTV "); Serial.println(real_table_value_x, 4);}
      Serial.print("DEBUG: "); Serial.println("EOM");
      return real_table_value_y;
    }
    
    // case 2: this is the first row and the independent variable is smaller than it
    // therefore extrapolation backward is required
    else if((row == 1) && (real_table_value_x > independent_variable_value)){
      //Serial.print("  DEBUG: C2 RTV "); Serial.println(real_table_value_x, 4);
      // look up the value in row 1 to calculate the slope to extrapolate
      previous_real_table_value_x = real_table_value_x;
      previous_real_table_value_y = real_table_value_y;
      
      getTableRow(sensorIndex, row++, &xval, &yval);
      real_table_value_x = x_scaler * xval;
      real_table_value_y = y_scaler * yval;
      
      slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
      //Serial.print("DEBUG: "); Serial.println("EOM");
      return previous_real_table_value_y - slope * (previous_real_table_value_x - independent_variable_value);
    }
    
    // case 3: the independent variable is between the current row and the previous row
    // interpolation is required  
    else if((row > 1) && (real_table_value_x > independent_variable_value) && (independent_variable_value > previous_real_table_value_x)){
      //Serial.print("  DEBUG: C3 RTV "); Serial.println(real_table_value_x, 4);

      // interpolate between the two values
      slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
      
      //Serial.print("DEBUG: "); Serial.println("EOM");
      return previous_real_table_value_y + (independent_variable_value - previous_real_table_value_x) * slope;
    }
    
    // store into the previous values for use in interpolation/extrapolation
    previous_real_table_value_x = real_table_value_x;
    previous_real_table_value_y = real_table_value_y;
  }
  
  // case 4: the independent variable is greater than the largest value in the table, so we need to extrapolate forward
  // if you got through the entire table without an early return that means the independent_variable_value
  // the last values stored should be used for the slope calculation
  //Serial.print("  DEBUG: C4 RTV "); Serial.println(real_table_value_x, 4);

  slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
  //Serial.print("DEBUG: "); Serial.println("EOM");
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
  if (sensorIndex == 1) { Serial.print("    DEBUG: GTR "); Serial.println(row_number);} 
  *xval = getTableValue(sensorIndex, row_number, 0);
  *yval = getTableValue(sensorIndex, row_number, 1);
  if (sensorIndex == 1) { Serial.print("    DEBUG: GTR BOOL");Serial.println(*xval != INTERPOLATION_TERMINATOR && *yval != INTERPOLATION_TERMINATOR);}
  return (*xval != INTERPOLATION_TERMINATOR && *yval != INTERPOLATION_TERMINATOR);
}

