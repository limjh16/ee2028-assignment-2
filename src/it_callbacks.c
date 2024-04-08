#include "main.h"
#include "i2c.h"
#include "it_flags.h"
#include <lis3mdl_magneto.h>
#include <lps22hb_baro.h>
#include <hts221_temphum.h>
#include <lsm6dsl_gyro.h>
#include <stdio.h>
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
	case LSM3MDL_DRDY_EXTI8_Pin:
		sensor_callbacks |= MAG;
		break;

	case LPS22HB_INT_DRDY_EXTI0_Pin:
		sensor_callbacks |= PRESSURE;
		break;

	case HTS221_DRDY_EXTI15_Pin:
		sensor_callbacks |= TEMP;
		break;

	case LSM6DSL_INT1_EXTI11_Pin:
		sensor_callbacks |= GYRO;
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
void Button_Handler(void)
{
	laser_charge += 3;
	new_laser = 1;
	uint32_t timenow = HAL_GetTick();
	if (button_count == 0 || timenow - button_time > 500)
	{
		button_count = 1;
		button_time = timenow;
	}
	else if (button_count == 1)
	{
		button_count = 0;
		change_mode = 1;
		if (global_mode)
			global_mode = DEFEND_MODE;
		else
			global_mode = SENTRY_MODE;
		button_time = timenow;
	}
}
