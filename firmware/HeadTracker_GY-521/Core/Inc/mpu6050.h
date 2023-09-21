/*
 * mpu6050.h
 *
 *  Created on: Sep 19, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

typedef struct
{
  int16_t raw_acc_x;
  int16_t raw_acc_y;
  int16_t raw_acc_z;
  int16_t raw_temp;

  int16_t raw_gyro_x;
  int16_t raw_gyro_y;
  int16_t raw_gyro_z;

  float gyro_x_offset;
  float gyro_y_offset;
  float gyro_z_offset;

  float temp;
  float acc_x;
  float acc_y;
  float acc_z;
  float gyro_x;
  float gyro_y;
  float gyro_z;

  float angle_gyro_x;
  float angle_gyro_y;
  float angle_gyro_z;
  float angle_acc_x;
  float angle_acc_y;
  float angle_acc_z;

  float angle_x;
  float angle_y;
  float angle_z;

  float interval;
  long  pre_interval;

  float acc_coef;
  float gyro_coef;

} mpu6050_t;

int init_mpu6050(I2C_HandleTypeDef* i2c_handle, mpu6050_t* mpu6050);
void update_mpu6050(I2C_HandleTypeDef* i2c_handle, mpu6050_t* mpu6050);

#endif /* MPU6050_H */
