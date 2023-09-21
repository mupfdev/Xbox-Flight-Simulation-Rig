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

#define QMC5883L_ADDR (0x0d << 1)
#define DATA_OUT_REG  0x00;
#define DATA_OUT_SIZE 6U
#define CHIP_ID_REG   0x0d
#define CHIP_ID_SIZE  1U

typedef struct
{
  int16_t raw_data_x;
  int16_t raw_data_y;
  int16_t raw_data_z;

} qmc5883l_t;

static qmc5883l_t* qmc5883l;

static       osThreadId_t qmc5883l_task_handle;
static const osThreadAttr_t qmc5883l_task_attributes = {
  .name = "qmc5883l",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void qmc5883l_task(void *i2c_handle);
static void update_qmc5883l(I2C_HandleTypeDef* i2c_handle);

int init_qmc5883l(I2C_HandleTypeDef* i2c_handle)
{
  HAL_StatusTypeDef status;
  uint8_t           chip_id;
  uint8_t           data;

  memset(qmc5883l, 0, sizeof(qmc5883l_t));

  status = HAL_I2C_Mem_Read(i2c_handle, QMC5883L_ADDR, CHIP_ID_REG, 1, &chip_id, CHIP_ID_SIZE, 100);
  if (HAL_OK != status)
  {
    qmc5883l_task_handle = osThreadNew(qmc5883l_task, (void*)i2c_handle, &qmc5883l_task_attributes);

    return 1;
  }

  if (0xff == chip_id)
  {
    return 0;
  }

  return 1;
}

static void qmc5883l_task(void *i2c_handle)
{
  while (1)
  {
    update_qmc5883l((I2C_HandleTypeDef*)i2c_handle);
    osDelay(1);
  }
}

static void update_qmc5883l(I2C_HandleTypeDef* i2c_handle)
{
}
