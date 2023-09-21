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

void calibrate_qmc58831(void);
void get_data_from_qmc5883l(int16_t* x, int16_t* y, int16_t* z);
int  init_qmc5883l(I2C_HandleTypeDef* i2c_handle);

#endif /* QMC5883L_H */
