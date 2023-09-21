/*
 * mpu6050.c
 *
 *  Created on: Sep 19, 2023
 *      Author: Michael Fitzmayer
 */

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "mpu6050.h"
#include "stm32f1xx_hal.h"

#define MPU6050_ADDR         (0x68 << 1)
#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG       0x1a
#define MPU6050_gyro_CONFIG  0x1b
#define MPU6050_acc_EL_CONFIG 0x1c
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_PWR_MGMT_1   0x6b
#define MPU6050_TEMP_H       0x41
#define MPU6050_TEMP_L       0x42

int init_mpu6050(I2C_HandleTypeDef* i2c_handle, mpu6050_t* mpu6050)
{
  HAL_StatusTypeDef status;
  uint8_t           self_check;
  uint8_t           data;

  memset(mpu6050, 0, sizeof(mpu6050_t));

  status = HAL_I2C_Mem_Read(i2c_handle, MPU6050_ADDR, MPU6050_WHO_AM_I, 1, &self_check, 1, 100);
  if (HAL_OK != status)
  {
    return 1;
  }

  if (0x68 == self_check)
  {
    float x = 0;
    float y = 0;
    float z = 0;

    data = 0x00;
    HAL_I2C_Mem_Write(i2c_handle, MPU6050_ADDR, MPU6050_SMPLRT_DIV,   1, &data, 1, 100);
    HAL_I2C_Mem_Write(i2c_handle, MPU6050_ADDR, MPU6050_CONFIG,       1, &data, 1, 100);
    data = 0x08;
    HAL_I2C_Mem_Write(i2c_handle, MPU6050_ADDR, MPU6050_gyro_CONFIG,  1, &data, 1, 100);
    data = 0x00;
    HAL_I2C_Mem_Write(i2c_handle, MPU6050_ADDR, MPU6050_acc_EL_CONFIG, 1, &data, 1, 100);
    data = 0x01;
    HAL_I2C_Mem_Write(i2c_handle, MPU6050_ADDR, MPU6050_PWR_MGMT_1,   1, &data, 1, 100);

    update_mpu6050(i2c_handle, mpu6050);

    for(int i = 0; i < 3000; i++)
    {
      int16_t rx;
      int16_t ry;
      int16_t rz;
      uint8_t buf[6];

      HAL_I2C_Mem_Read(i2c_handle, MPU6050_ADDR, 0x43, 1, buf, 6, 100);

      rx = (int16_t)(buf[0] << 8 | buf[1]);
      ry = (int16_t)(buf[2] << 8 | buf[3]);
      rz = (int16_t)(buf[4] << 8 | buf[5]);

      x += ((float)rx) / 65.5;
      y += ((float)ry) / 65.5;
      z += ((float)rz) / 65.5;
    }

    mpu6050->gyro_x_offset = x / 3000;
    mpu6050->gyro_y_offset = y / 3000;
    mpu6050->gyro_z_offset = z / 3000;
    mpu6050->acc_coef      = 0.02f;
    mpu6050->gyro_coef     = 0.98f;

    update_mpu6050(i2c_handle, mpu6050);

    return 0;
  }

  return 1;
}

void update_mpu6050(I2C_HandleTypeDef* i2c_handle, mpu6050_t* mpu6050)
{
  uint32_t millis;
  uint8_t  data[14];

  HAL_I2C_Mem_Read(i2c_handle, MPU6050_ADDR, 0x3b, 1, data, 14, 100);

  mpu6050->raw_acc_x  = (int16_t)(data[0]  << 8 | data[1]);
  mpu6050->raw_acc_y  = (int16_t)(data[2]  << 8 | data[3]);
  mpu6050->raw_acc_z  = (int16_t)(data[4]  << 8 | data[5]);
  mpu6050->raw_temp   = (int16_t)(data[6]  << 8 | data[7]);
  mpu6050->raw_gyro_x = (int16_t)(data[8]  << 8 | data[9]);
  mpu6050->raw_gyro_y = (int16_t)(data[10] << 8 | data[11]);
  mpu6050->raw_gyro_z = (int16_t)(data[12] << 8 | data[13]);

  mpu6050->temp = (mpu6050->raw_temp + 12412.0) / 340.0;

  mpu6050->acc_x = ((float)mpu6050->raw_acc_x) / 16384.0;
  mpu6050->acc_y = ((float)mpu6050->raw_acc_y) / 16384.0;
  mpu6050->acc_z = ((float)mpu6050->raw_acc_z) / 16384.0;

  mpu6050->angle_acc_x = atan2(mpu6050->acc_y, sqrt(mpu6050->acc_z * mpu6050->acc_z + mpu6050->acc_x * mpu6050->acc_x)) * 360 / 2.0 / M_PI;
  mpu6050->angle_acc_y = atan2(mpu6050->acc_x, sqrt(mpu6050->acc_z * mpu6050->acc_z + mpu6050->acc_y * mpu6050->acc_y)) * 360 / -2.0 / M_PI;

  mpu6050->gyro_x = ((float)mpu6050->raw_gyro_x) / 65.5;
  mpu6050->gyro_y = ((float)mpu6050->raw_gyro_y) / 65.5;
  mpu6050->gyro_z = ((float)mpu6050->raw_gyro_z) / 65.5;

  mpu6050->gyro_x -= mpu6050->gyro_x_offset;
  mpu6050->gyro_y -= mpu6050->gyro_y_offset;
  mpu6050->gyro_z -= mpu6050->gyro_z_offset;

  millis            = HAL_GetTick();
  mpu6050->interval = (millis - mpu6050->pre_interval) * 0.001;

  mpu6050->angle_gyro_x += mpu6050->gyro_x * mpu6050->interval;
  mpu6050->angle_gyro_y += mpu6050->gyro_y * mpu6050->interval;
  mpu6050->angle_gyro_z += mpu6050->gyro_z * mpu6050->interval;

  mpu6050->angle_x = (mpu6050->gyro_coef * (mpu6050->angle_x + mpu6050->gyro_x * mpu6050->interval)) + (mpu6050->acc_coef * mpu6050->angle_acc_x);
  mpu6050->angle_y = (mpu6050->gyro_coef * (mpu6050->angle_y + mpu6050->gyro_y * mpu6050->interval)) + (mpu6050->acc_coef * mpu6050->angle_acc_y);
  mpu6050->angle_z = mpu6050->angle_z;

  millis                = HAL_GetTick();
  mpu6050->pre_interval = millis;
}
