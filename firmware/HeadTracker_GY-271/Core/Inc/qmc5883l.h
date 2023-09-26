/*
 * qmc5883l.h
 *
 *  Created on: Sep 21, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef QMC5883L_H
#define QMC5883L_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define CONTROL_1_MODE_STANDBY    0x00
#define CONTROL_1_MODE_CONTINUOUS 0x01
#define CONTROL_1_ODR_10HZ        0x00
#define CONTROL_1_ODR_50HZ        0x04
#define CONTROL_1_ODR_100HZ       0x08
#define CONTROL_1_ODR_200HZ       0x0c
#define CONTROL_1_RNG_2G          0x00
#define CONTROL_1_RNG_8G          0x10
#define CONTROL_1_OSR_512         0x00
#define CONTROL_1_OSR_256         0x40
#define CONTROL_1_OSR_128         0x80
#define CONTROL_1_OSR_64          0xc0

#define CONTROL_2_INT_ENB_ENABLE  0x00
#define CONTROL_2_INT_ENB_DISABLE 0x01
#define CONTROL_2_ROL_PNT_NORMAL  0x00
#define CONTROL_2_ROL_PNT_ENABLE  0x40
#define CONTROL_2_SOFT_RST_NORMAL 0x00
#define CONTROL_2_SOFT_RST_RESET  0x80

void calibrate_qmc5883l(void);
void get_data_from_qmc5883l(int16_t* x, int16_t* y, int16_t* z);
void get_raw_data_from_qmc5883l(int16_t* x, int16_t* y, int16_t* z);
int  init_qmc5883l(I2C_HandleTypeDef* i2c_handle, uint8_t control_1, uint8_t control_2);

#endif /* QMC5883L_H */
