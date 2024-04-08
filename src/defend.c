#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "it_flags.h"
#include <lis3mdl_magneto.h>
#include <lps22hb_baro.h>
#include <hts221_temphum.h>
#include <lsm6dsl_gyro.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

void defend_function(void)
{
	static uint8_t new_data = 0;
	static int16_t accel_data[3];
	static unsigned long good_accel_count;
	if (change_mode)
	{
		LSM6DSL_Req(&hi2c2); // wake up LSM6
		laser_charge = 0;
		change_mode = 0;
		new_laser = 0;
		good_accel_count = 0;
		/*Configure GPIO pin : PtPin */
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		/*Configure GPIO pin Output Level */
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
	}
	if (rtc_flag)
	{
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		printf("WARNING!\r\n");
		rtc_flag = 0;
	}
	if (new_laser)
	{
		new_laser = 0;
		if (laser_charge > 10)
			laser_charge = 10;
		if (laser_charge > 5)
		{
			laser_charge -= 5;
			printf("SHOOT LASER PEWPEWPEW\r\n");
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
		}
		printf("Laser Charge: %d\r\n", laser_charge);
	}
	if (sensor_callbacks & ACCEL || sensor_callbacks & GYRO)
	{
		LSM6DSL_Int_Callback(&hi2c2);
		// printf("%d %d ACCEL\n", sensor_counter, sensor_callbacks);
		sensor_callbacks &= ~(ACCEL | GYRO);
		new_data |= GYRO;
	}
	while (sensor_counter--)
	{
		if (new_data & ACCEL || new_data & GYRO)
		{
			ACCEL_ProcessXYZ(accel_data);
			new_data &= ~(ACCEL | GYRO);
			// printf("%d\r\n", accel_data[2]);
			if (
				sqrt(
					accel_data[0] * accel_data[0] +
					accel_data[1] * accel_data[1] +
					accel_data[2] * accel_data[2]) > 1500 ||
				accel_data[2] > -700)
			{
				good_accel_count = 0;
			}
			else
				++good_accel_count;
		}
	}
	if (good_accel_count > 50) // At ~200Hz, this is ~0.25s
	{
		// SHUTDOWN!
		LSM6DSL_Power_Down(&hi2c2);
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
		__disable_irq();
		printf("TERMINATE!\r\n");
		while(1);
	}
}
