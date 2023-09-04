/*
 * ads1115.c
 *
 *  Created on: Sep 3, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define DEV_ADDR 0x48

extern I2C_HandleTypeDef hi2c2;

void ads1115_init(void)
{
  HAL_StatusTypeDef status;
  uint8_t buffer[3];

  buffer[0] = 0x01;
  buffer[1] = 0xc3;
  buffer[2] = 0xe3;

  status = HAL_I2C_Master_Transmit(&hi2c2, DEV_ADDR << 1, buffer, 3, 50);
  if (HAL_OK != status)
  {
    return;
  }

  status = HAL_I2C_Master_Transmit(&hi2c2, DEV_ADDR << 1, buffer, 1, 50);
  if (HAL_OK != status)
  {
    return;
  }

  status = HAL_I2C_Master_Receive(&hi2c2, DEV_ADDR << 1, buffer, 3, 50);
  if (HAL_OK != status)
  {
    return;
  }
}

void ads1115_read(int16_t* raw, int16_t lower_limit, int16_t upper_limit)
{
  HAL_StatusTypeDef status;
  uint8_t buffer[2];

  buffer[0] = 0x00;

  status = HAL_I2C_Master_Transmit(&hi2c2, DEV_ADDR << 1, buffer, 1, 50);
  if (HAL_OK != status)
  {
    return;
  }

  status = HAL_I2C_Master_Receive(&hi2c2, DEV_ADDR << 1, buffer, 2, 50);
  if (HAL_OK != status)
  {
    return;
  }

  *raw = ((int16_t)buffer[0] << 8) | buffer[1];

  if (*raw >= upper_limit)
  {
    *raw = upper_limit;
  }
  else if (*raw <= lower_limit)
  {
    *raw = lower_limit;
  }
}

void ads1115_set_channel(uint8_t channel)
{
  HAL_StatusTypeDef status;
  uint8_t buffer[3];

  buffer[0] = 0x01;
  buffer[2] = 0xe3;

  switch (channel)
  {
    default:
    case 0:
      buffer[1] = 0xc3;
      break;
    case 1:
      buffer[1] = 0xd3;
      break;
    case 2:
      buffer[1] = 0xe3;
      break;
    case 3:
      buffer[1] = 0xf3;
      break;
  }

  status = HAL_I2C_Master_Transmit(&hi2c2, DEV_ADDR << 1, buffer, 3, 50);
  if (HAL_OK != status)
  {
    __NOP();
  }
}
