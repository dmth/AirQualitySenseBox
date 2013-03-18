//Interpolates the resitance to a ppm value.

// Code from SensorShield
//Taken from: https://github.com/WickedDevice/aqe_sensor_interface_shield/blob/master/src/interpolation.c
#define INTERPOLATION_X_INDEX 0
#define INTERPOLATION_Y_INDEX 1
#define INTERPOLATION_TERMINATOR 0xff

// these are the conversion factors required to turn into floating point values (multiply table values by these)
const float x_scaler[2] = {0.4f, 0.003f};
const float y_scaler[2] = {1.7f, 165.0f};
const float independent_scaler[2] = {0.0001f, 0.0004f};
const uint32_t independent_scaler_inverse[2] = { 10000, 2500 };

// get_x_or_get_y = 0 returns x value from table, get_x_or_get_y = 1 returns y value from table
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

uint8_t * get_p_x_scaler(uint8_t sensor_index){
    return (uint8_t *) &(x_scaler[sensor_index]);
}

uint8_t * get_p_y_scaler(uint8_t sensor_index){
    return (uint8_t *) &(y_scaler[sensor_index]);
}

uint8_t * get_p_independent_scaler(uint8_t sensor_index){
    return (uint8_t *) &(independent_scaler[sensor_index]);
}

uint32_t get_independent_scaler_inverse(uint8_t sensor_index){
    return independent_scaler_inverse[sensor_index];
}

//Code from Eggbus
uint8_t buffer[16];

/*
returns the computed sensor value of the index as a 32-bit integer
*/
float getSensorValue(uint8_t sensorIndex, uint32_t measurement){
  float slope = 0.0;
  float x_scaler = getTableXScaler(sensorIndex);
  float y_scaler = getTableYScaler(sensorIndex);
  float i_scaler = getIndependentScaler(sensorIndex);
  uint32_t measured_value = measurement;
  
  float independent_variable_value = (i_scaler * measured_value);
  uint8_t xval, yval, row = 0;
  float real_table_value_x, real_table_value_y;
  float previous_real_table_value_x = 0.0, previous_real_table_value_y = 0.0;
    
  while(getTableRow(sensorIndex, row++, &xval, &yval)){
    real_table_value_x = x_scaler * xval;
    real_table_value_y = y_scaler * yval;
    
    // case 1: this row is an exact match, just return the table value for y
    if(real_table_value_x == independent_variable_value){
      return real_table_value_y;
    }
    
    // case 2: this is the first row and the independent variable is smaller than it
    // therefore extrapolation backward is required
    else if((row == 1)
      && (real_table_value_x > independent_variable_value)){

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
    else if((row > 1)
      && (real_table_value_x > independent_variable_value)
      && (independent_variable_value > previous_real_table_value_x)){
      // interpolate between the two values
      slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
      return previous_real_table_value_y + (independent_variable_value - previous_real_table_value_x) * slope;
    }
    
    // store into the previous values for use in interpolation/extrapolation
    previous_real_table_value_x = real_table_value_x;
    previous_real_table_value_y = real_table_value_y;
  }
  
  // case 4: the independent variable is must be greater than the largest value in the table, so we need to extrapolate forward
  // if you got through the entire table without an early return that means the independent_variable_value
  // the last values stored should be used for the slope calculation
  slope = (real_table_value_y - previous_real_table_value_y) / (real_table_value_x - previous_real_table_value_x);
  return real_table_value_y + slope * (independent_variable_value - real_table_value_x);
}

uint32_t buf_to_value(uint8_t * buf){
  uint8_t index = 1;
  uint32_t ret = buf[0];
  for(index = 1; index < 4; index++){
    ret = (ret << 8); // make space for the next byte
    ret = (ret | buf[index]); // slide in the next byte
  }
  
  return ret;
}

#define SENSOR_DATA_BASE_OFFSET (32)
#define SENSOR_DATA_ADDRESS_BLOCK_SIZE (256) 
#define SENSOR_TYPE_FIELD_OFFSET (0)
#define SENSOR_UNITS_FIELD_OFFSET (16)
#define SENSOR_R0_FIELD_OFFSET (32)
#define SENSOR_MEASURED_INDEPENDENT_OFFSET (36)
#define SENSOR_TABLE_X_SCALER_OFFSET (40)
#define SENSOR_RAW_VALUE_FIELD_OFFSET (44)
#define SENSOR_TABLE_Y_SCALER_OFFSET (48)
#define SENSOR_INDEPENDENT_SCALER_OFFSET (52)
#define SENSOR_COMPUTED_MAPPING_TABLE_OFFSET (56)
#define SENSOR_COMPUTED_MAPPING_TABLE_ROW_SIZE (8)
#define SENSOR_COMPUTED_MAPPING_TABLE_TERMINATOR (0xff)

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
gets the requested table row for the requested sensor
*/

//return false if buffer[0] | [1] = 0xff
bool getTableRow(uint8_t sensorIndex, uint8_t row_number, uint8_t * xval, uint8_t *yval){
  *xval = getTableValue(sensorIndex, row_number, 0);
  *yval = getTableValue(sensorIndex, row_number, 1);
  return (*xval != SENSOR_COMPUTED_MAPPING_TABLE_TERMINATOR && *yval != SENSOR_COMPUTED_MAPPING_TABLE_TERMINATOR);
}

/*
gets the independent variable measure for the requested sensor
*/
//uint32_t getSensorIndependentVariableMeasure(uint8_t sensorIndex){
//  i2cGetValue(currentBusAddress, SENSOR_DATA_BASE_OFFSET + sensorIndex * SENSOR_DATA_ADDRESS_BLOCK_SIZE + SENSOR_MEASURED_INDEPENDENT_OFFSET, 4);
//  return buf_to_value(buffer);
//}


float buf_to_fvalue(uint8_t * buf){
  float returnValue = 0;
  uint32_t ret = buf[0];
  uint8_t index = 1;
  for(index = 1; index < 4; index++){
    ret = (ret << 8); // make space for the next byte
    ret = (ret | buf[index]); // slide in the next byte
  }
  memcpy(&returnValue, &ret, 4);
  return returnValue;
}

