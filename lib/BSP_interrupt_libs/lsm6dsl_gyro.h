#ifndef __STM32L475E_IOT01_GYRO_H
#define __STM32L475E_IOT01_GYRO_H


#ifndef __STM32L475E_IOT01_ACCELERO_H
#define __STM32L475E_IOT01_ACCELERO_H

#define LSM6DSL_DRDY_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOD_CLK_ENABLE()

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Include Gyro component driver */
#include "lsm6dsl.h"
#include "stm32l4xx_hal.h"

#ifndef I2C_DEFAULT_TIMEOUT
#define I2C_DEFAULT_TIMEOUT 100
#endif
#define LSM6DSL_INT1_EXTI11_Pin GPIO_PIN_11
#define LSM6DSL_INT1_EXTI11_GPIO_Port GPIOD

#define LSM6DSL_GYRO_DRDY_BYPASS_EN 0x00
#define LSM6DSL_GYRO_DRDY_CONTINUOUS_EN 0b110

#define LSM6DSL_GYRO_DRDY_INT1_EN 0x02
#define LSM6DSL_ACC_DRDY_INT1_EN 0x01

#define LSM6DSL_ACC_GYRO_SW_RESET 0x01

#define LSM6DSL_GYRO_SLEEP_EN 0x40
#define LSM6DSL_INT2_ON_INT1 0x20
#define LSM6DSL_DRDY_MASK 0x08

#define MSBLSB(arr, i) ((((uint16_t)arr[2 * i + 1]) << 8) | (uint16_t)arr[2 * i])

typedef enum
{
  GYRO_OK = 0,
  GYRO_ERROR = 1,
  GYRO_TIMEOUT = 2
}
GYRO_StatusTypeDef;

typedef enum
{
  ACCELERO_OK = 0,
  ACCELERO_ERROR = 1,
  ACCELERO_TIMEOUT = 2
}
ACCELERO_StatusTypeDef;

GYRO_StatusTypeDef LSM6DSL_Init_IntMode(I2C_HandleTypeDef *hi2c);
void GYRO_ProcessXYZ(float* pfData);
float GYRO_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold);
void ACCEL_ProcessXYZ(int16_t *pDataXYZ);
float ACCEL_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold);
HAL_StatusTypeDef LSM6DSL_Req(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef LSM6DSL_Int_Callback(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef LSM6DSL_Power_Down(I2C_HandleTypeDef *hi2c);


#ifdef __cplusplus
}
#endif

#endif /* __STM32L475E_IOT01_GYRO_H */
#endif /* __STM32L475E_IOT01_ACCELERO_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
