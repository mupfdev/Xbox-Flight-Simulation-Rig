/*
 * qmc5883l.c
 *
 *  Created on: Sep 21, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "qmc5883l.h"

#define QMC5883L_ADDR             (0x0d << 1)

#define DATA_OUT_REG              0x00
#define DATA_OUT_SIZE             6U
#define CHIP_ID_REG               0x0d
#define CHIP_ID_OK                0xff
#define CHIP_ID_SIZE              1U
#define STATUS_REG                0x06
#define STATUS_SIZE               1U
#define CONTROL_1_REG             0x09
#define CONTROL_1_SIZE            1U
#define CONTROL_2_REG             0x0a
#define CONTROL_2_SIZE            1U
#define SET_RESET_PERIOD_REG      0x0b
#define SET_RESET_PERIOD_SIZE     1U

#define STATUS_DRDY               0x00
#define STATUS_OVL                0x01
#define STATUS_DOR                0x02

#define X_AXIS                    0
#define Y_AXIS                    1
#define Z_AXIS                    2

typedef struct
{
  int16_t raw[3];
  int16_t calib[3];
  int16_t axis[3];

} qmc5883l_t;

static qmc5883l_t qmc5883l;

static       osThreadId_t qmc5883l_task_handle;
static const osThreadAttr_t qmc5883l_task_attributes = {
  .name = "qmc5883l",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void qmc5883l_task(void *i2c_handle);
static void update_qmc5883l(I2C_HandleTypeDef* i2c_handle);

void calibrate_qmc5883l(void)
{
  for (int i = X_AXIS; i <= Z_AXIS; i += 1)
  {
    qmc5883l.calib[i] = qmc5883l.raw[i];
  }
}

void get_data_from_qmc5883l(int16_t* x, int16_t* y, int16_t* z)
{
  if (NULL != x)
  {
    *x = qmc5883l.axis[X_AXIS];
  }

  if (NULL != y)
  {
    *y = qmc5883l.axis[Y_AXIS];;
  }

  if (NULL != z)
  {
    *z = qmc5883l.axis[Z_AXIS];;
  }
}

void get_raw_data_from_qmc5883l(int16_t* x, int16_t* y, int16_t* z)
{
  if (NULL != x)
  {
    *x = qmc5883l.raw[X_AXIS];
  }

  if (NULL != y)
  {
    *y = qmc5883l.raw[Y_AXIS];;
  }

  if (NULL != z)
  {
    *z = qmc5883l.raw[Z_AXIS];;
  }
}

int init_qmc5883l(I2C_HandleTypeDef* i2c_handle, uint8_t control_1, uint8_t control_2)
{
  HAL_StatusTypeDef status;
  uint8_t           chip_id = 0;

  status = HAL_I2C_Mem_Read(i2c_handle, QMC5883L_ADDR, CHIP_ID_REG, 1, &chip_id, CHIP_ID_SIZE, 100);
  if (HAL_OK != status)
  {
    return 1;
  }

  if (CHIP_ID_OK == chip_id)
  {
    uint8_t data;

    data = control_1;

    HAL_I2C_Mem_Write(i2c_handle, QMC5883L_ADDR, CONTROL_1_REG, 1, &data, CONTROL_1_SIZE, 100);

    data = control_2;

    HAL_I2C_Mem_Write(i2c_handle, QMC5883L_ADDR, CONTROL_2_REG, 1, &data, CONTROL_2_SIZE, 100);

    /* It is recommended that the register 0BH is written by 0x01. */
    data = 0x01;

    HAL_I2C_Mem_Write(i2c_handle, QMC5883L_ADDR, SET_RESET_PERIOD_REG, 1, &data, SET_RESET_PERIOD_SIZE, 100);

    qmc5883l_task_handle = osThreadNew(qmc5883l_task, (void*)i2c_handle, &qmc5883l_task_attributes);

    return 0;
  }

  return 1;
}

static void qmc5883l_task(void *i2c_handle)
{
  update_qmc5883l((I2C_HandleTypeDef*)i2c_handle);
  calibrate_qmc5883l();

  while (1)
  {
    update_qmc5883l((I2C_HandleTypeDef*)i2c_handle);
    osDelay(1);
  }
}

static void update_qmc5883l(I2C_HandleTypeDef* i2c_handle)
{
  HAL_StatusTypeDef status;
  uint8_t           data[DATA_OUT_SIZE] = { 0 };

  status = HAL_I2C_Mem_Read(i2c_handle, QMC5883L_ADDR, DATA_OUT_REG, 1, data, DATA_OUT_SIZE, 100);

  if (HAL_OK == status)
  {
    qmc5883l.raw[X_AXIS] = ((int16_t)data[1] << 8) | data[0];
    qmc5883l.raw[Y_AXIS] = ((int16_t)data[3] << 8) | data[2];
    qmc5883l.raw[Z_AXIS] = ((int16_t)data[5] << 8) | data[4];

    for (int i = X_AXIS; i <= Z_AXIS; i += 1)
    {
      int32_t tmp = qmc5883l.raw[i] - qmc5883l.calib[i];

      if (tmp < INT16_MIN)
      {
        qmc5883l.axis[i] = (INT16_MAX - qmc5883l.raw[i]) + (qmc5883l.calib[i] - INT16_MIN);
      }
      else if (tmp > INT16_MAX)
      {
        qmc5883l.axis[i] = (INT16_MAX - qmc5883l.calib[i]) + (qmc5883l.raw[i] - INT16_MIN);
      }
      else
      {
        qmc5883l.axis[i] = (int16_t)tmp;
      }
    }
  }
}
