
#ifndef INC_HTS221_TEMPHUM_H_
#define INC_HTS221_TEMPHUM_H_


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "hts221.h"
#include "stm32l4xx_hal.h"
// #include "stm32l475e_iot01.h"

/* Exported types ------------------------------------------------------------*/
#define HTS221_I2C_ADDRESS   (uint8_t)0xBE
#define HTS221_TH_AUTO_INCR 0x80

#define HTS221_TH_ACTIVE_MODE 0x80
#define HTS221_TH_BDU 0x04
#define HTS221_TH_ONESHOT 0x00
#define HTS221_TH_1HZ 0x01

#define HTS221_TH_ONESHOT_EN 0x01

#define HTS221_TH_DRDY_EN 0x04

#ifndef I2C_DEFAULT_TIMEOUT
#define I2C_DEFAULT_TIMEOUT 100
#endif
#define HTS221_DRDY_EXTI15_Pin GPIO_PIN_15
#define HTS221_DRDY_EXTI15_GPIO_Port GPIOD
#define MSBLSB(arr, i) ((((uint16_t)arr[2 * i + 1]) << 8) | (uint16_t)arr[2 * i])
/**
  * @brief  TSENSOR Status
  */
typedef enum
{
  TSENSOR_OK = 0,
  TSENSOR_ERROR = 1
} TSENSOR_Status_TypDef;

/** @defgroup STM32L475E_IOT01_TEMPERATURE_Exported_Functions TEMPERATURE Exported Constants
  * @{
  */
/* Exported macros -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/* Sensor Configuration Functions */
TSENSOR_Status_TypDef TH_Init_IntMode(I2C_HandleTypeDef *hi2c);
float T_Process(volatile uint8_t *telem_monitor, float threshold);
float H_Process(volatile uint8_t *telem_monitor, float threshold);
HAL_StatusTypeDef TH_Req(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef TH_Int_Callback(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef TH_Power_Down(I2C_HandleTypeDef *hi2c);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* INC_HTS221_TEMPHUM_H_ */

 /************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
