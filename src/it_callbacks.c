#include "main.h"
#include "i2c.h"
#include "it_flags.h"
#include <lis3mdl_magneto.h>
#include <lps22hb_baro.h>
#include <hts221_temphum.h>
#include <stdio.h>
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
	case LSM3MDL_DRDY_EXTI8_Pin:
		MAGNETO_Int_Callback(&hi2c2);
		break;

	case LPS22HB_INT_DRDY_EXTI0_Pin:
		BARO_PSENSOR_Int_Callback(&hi2c2);
		break;

	case HTS221_DRDY_EXTI15_Pin:
		TH_Int_Callback(&hi2c2);
		break;

	case BUTTON_EXTI13_Pin:
		Button_Handler();
		break;

	default:
		printf("Undefined GPIO Interrupt!\n");
		break;
	}
}
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	++sensor_counter;
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	printf("DMA I2C Error\n");
}
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
	rtc_flag = 1;
}
