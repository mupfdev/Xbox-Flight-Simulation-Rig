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

int init_qmc5883l(I2C_HandleTypeDef* i2c_handle);

#endif /* QMC5883L_H */
