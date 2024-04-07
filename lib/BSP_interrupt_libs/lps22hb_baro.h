/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L475E_IOT01_PSENSOR_H
#define __STM32L475E_IOT01_PSENSOR_H

#define LPS22HB_DRDY_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOD_CLK_ENABLE()
#define LPS22HB_DRDY_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOD_CLK_DISABLE()

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lps22hb.h"
#include "stm32l4xx_hal.h"

/**
  * @brief  PSENSOR Status
  */

#define LPS22HB_BARO_POWER_DOWN_MODE 0x00
#define LPS22HB_BARO_BDU 0x02

#define LPS22HB_BARO_ONESHOT_EN 0x01
#define LPS22HB_BARO_AUTO_INCR_EN 0x10

#define LPS22HB_BARO_INT_ACTIVE_HIGH 0x00
#define LPS22HB_BARO_INT_PUSH_PULL 0x00
#define LPS22HB_BARO_INT_DRDY_EN 0x04

#define LPS22HB_I2C_ADDRESS (uint8_t)0xBA
#define I2C_DEFAULT_TIMEOUT 100
#define LPS22HB_INT_DRDY_EXTI0_Pin GPIO_PIN_10
#define LPS22HB_INT_DRDY_EXTI0_GPIO_Port GPIOD
//#define LPS22HB_SLAVE_ADDRESS 0xB8

typedef enum
{
  PSENSOR_OK = 0,
  PSENSOR_ERROR = 1
}PSENSOR_Status_TypDef;

/**
  * @}
  */

/** @defgroup STM32L475E_IOT01_PRESSURE_Exported_Functions PRESSURE Exported Functions
  * @{
  */
/* Sensor Configuration Functions */
PSENSOR_Status_TypDef BARO_PSENSOR_Init(I2C_HandleTypeDef *hi2c);
float BARO_PSENSOR_Process(volatile uint8_t *telem_monitor, float threshold);
HAL_StatusTypeDef BARO_PSENSOR_Req(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BARO_PSENSOR_Int_Callback(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* __STM32L475E_IOT01_PSENSOR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
