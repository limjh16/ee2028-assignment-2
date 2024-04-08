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

void sentry_function(void)
{
	if (change_mode)
	{
		change_mode = 0;
		/*Configure GPIO pin : PtPin */
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
	}
	sensor_counter = 0;
	MAGNETO_Req(&hi2c2);
	BARO_PSENSOR_Req(&hi2c2);
	TH_Req(&hi2c2);
	LSM6DSL_Req(&hi2c2);
	while (sensor_counter < 4)
	{
		if (sensor_callbacks & TEMP || sensor_callbacks & HUMIDITY)
		{
			TH_Power_Down(&hi2c2);
			TH_Int_Callback(&hi2c2);
			// printf("%d %d TH\n", sensor_counter, sensor_callbacks);
			sensor_callbacks &= ~(TEMP | HUMIDITY);
		}
		if (sensor_callbacks & PRESSURE)
		{
			BARO_PSENSOR_Int_Callback(&hi2c2);
			// printf("%d %d BARO\n", sensor_counter, sensor_callbacks);
			sensor_callbacks &= ~PRESSURE;
		}
		if (sensor_callbacks & ACCEL || sensor_callbacks & GYRO)
		{
			LSM6DSL_Power_Down(&hi2c2);
			LSM6DSL_Int_Callback(&hi2c2);
			// printf("%d %d ACCEL\n", sensor_counter, sensor_callbacks);
			sensor_callbacks &= ~(ACCEL | GYRO);
		}
		if (sensor_callbacks & MAG)
		{
			MAGNETO_Int_Callback(&hi2c2);
			// printf("%d %d MAG\n", sensor_counter, sensor_callbacks);
			sensor_callbacks &= ~MAG;
		}
	}
	sensor_callbacks = 0;
	telem_monitor = 0;
	float temp = T_Process(&telem_monitor, THRESHOLD_TEMP);
	float pressure = BARO_PSENSOR_Process(&telem_monitor, THRESHOLD_PRESSURE);
	float humidity = H_Process(&telem_monitor, THRESHOLD_HUMIDITY);
	float accel = ACCEL_ProcessMagnitude(&telem_monitor, THRESHOLD_ACCEL);
	float gyro = GYRO_ProcessMagnitude(&telem_monitor, THRESHOLD_GYRO);
	float mag = MAGNETO_ProcessMagnitude(&telem_monitor, THRESHOLD_MAG);
	printf("--------------------------------\n");
	if (!telem_monitor) // TPHAGM
	{
		printf("T: %0.2f°C\r\n", temp);
		printf("P: %0.2f mbar\r\n", pressure);
		printf("H: %0.2f%%\r\n", humidity);
		printf("A: %0.2f mg (9.81 mm/s^2)\r\n", accel);
		printf("G: %0.2f m°/s\r\n", gyro);
		printf("M: %0.2f mgauss\r\n", mag);
	}
	else
	{
		if (telem_monitor & TEMP) // Temp
			printf("T: %0.2f°C, exceeds threshold of %0.2f°C\r\n", temp, (float)THRESHOLD_TEMP);

		if (telem_monitor & PRESSURE) // Pressure
			printf("P: %0.2f mbar, exceeds threshold of %0.2f mbar\r\n", pressure, (float)THRESHOLD_PRESSURE);

		if (telem_monitor & HUMIDITY) // Humidity
			printf("H: %0.2f%%, exceeds threshold of %0.2f%%\r\n", humidity, (float)THRESHOLD_HUMIDITY);

		if (telem_monitor & ACCEL) // Accel
			printf("A: %0.2f m/s^2, exceeds threshold of %0.2f mg (9.81 mm/s^2)\r\n", accel, (float)THRESHOLD_ACCEL);

		if (telem_monitor & GYRO) // Gyro
			printf("G: %0.2f rad/s, exceeds threshold of %0.2f m°/s\r\n", gyro, (float)THRESHOLD_GYRO);

		if (telem_monitor & MAG) // Mag
			printf("M: %0.2f mgauss, exceeds threshold of %0.2f mgauss\r\n", mag, (float)THRESHOLD_MAG);
	}
	printf("--------------------------------\n");

	HAL_SuspendTick();
	rtc_flag = 0;
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
	SystemClock_Config();
	HAL_ResumeTick();
	while (!rtc_flag && global_mode)
		;
	// HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
}
