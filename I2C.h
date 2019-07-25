/*
 * I2C.h
 *
 * Created: 19.07.2019 13:58:39
 *  Author: Alexander
 */ 


#ifndef I2C_H_
#define I2C_H_


void i2c_init(void);
void check_temperature(void);

extern int8_t temperature;

#endif /* I2C_H_ */