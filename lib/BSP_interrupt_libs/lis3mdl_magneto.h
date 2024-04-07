/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L475E_IOT01_MAGNETO_H
#define __STM32L475E_IOT01_MAGNETO_H

#define MAG_DRDY_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOC_CLK_ENABLE()
#define MAG_DRDY_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOC_CLK_DISABLE()

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lis3mdl.h"
#include "stm32l4xx_hal.h"

/* Exported types ------------------------------------------------------------*/
#define LIS3MDL_MAG_AUTO_INCR 0x80
#define I2C_DEFAULT_TIMEOUT 100
#define LSM3MDL_DRDY_EXTI8_Pin GPIO_PIN_8
#define LSM3MDL_DRDY_EXTI8_GPIO_Port GPIOC
typedef enum 
{
  MAGNETO_OK = 0,
  MAGNETO_ERROR = 1,
  MAGNETO_TIMEOUT = 2
} MAGNETO_StatusTypeDef;

MAGNETO_StatusTypeDef MAGNETO_Init_SingleMode(I2C_HandleTypeDef *hi2c);
void MAGNETO_ProcessXYZ(int16_t *pData);
float MAGNETO_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold);
HAL_StatusTypeDef MAGNETO_Req(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MAGNETO_Int_Callback(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif /* __STM32L475E_IOT01_MAGNETO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
