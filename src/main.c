/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"
#include "it_flags.h"
#include <lis3mdl_magneto.h>
#include <lps22hb_baro.h>
#include <hts221_temphum.h>
#include <lsm6dsl_gyro.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define THRESHOLD_TEMP 29
#define THRESHOLD_PRESSURE 1020
#define THRESHOLD_HUMIDITY 120
#define THRESHOLD_ACCEL 1000
#define THRESHOLD_GYRO 10000
#define THRESHOLD_MAG 1700

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	unsigned long lsm6dsl_boot_time = HAL_GetTick();
	sensor_callbacks = 0;
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_I2C2_Init();
	MX_USART1_UART_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */
	while (HAL_GetTick() - lsm6dsl_boot_time < 20)
		; // Boot-Up takes 15ms
	MAGNETO_Init_SingleMode(&hi2c2);
	BARO_PSENSOR_Init(&hi2c2);
	TH_Init_IntMode(&hi2c2);
	LSM6DSL_Init_IntMode(&hi2c2);
	printf("Peripherals Initialised\n");
	button_count = 0;
	global_mode = SENTRY_MODE;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		sensor_counter = 0;
		MAGNETO_Req(&hi2c2);
		BARO_PSENSOR_Req(&hi2c2);
		TH_Req(&hi2c2);
		LSM6DSL_Req(&hi2c2);
		while (sensor_counter < 4)
		{
			// if (HAL_GetTick() - lsm6dsl_boot_time > 2000)
			// {
			// 	lsm6dsl_boot_time = HAL_GetTick();
			// 	printf("%d %d\n", HAL_GPIO_ReadPin(LSM3MDL_DRDY_EXTI8_GPIO_Port, LSM3MDL_DRDY_EXTI8_Pin), HAL_GPIO_ReadPin(LPS22HB_INT_DRDY_EXTI0_GPIO_Port, LPS22HB_INT_DRDY_EXTI0_Pin));
			// }
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
		// int16_t pData[3];
		// MAGNETO_ProcessXYZ(pData);
		// printf("x: %d mgauss, y: %d mgauss, z: %d mgauss\n", pData[0], pData[1], pData[2]);
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
			printf("A: %0.2f cm/s^2\r\n", accel);
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
				printf("A: %0.2f m/s^2, exceeds threshold of %0.2f cm/s^2\r\n", accel, (float)THRESHOLD_ACCEL);

			if (telem_monitor & GYRO) // Gyro
				printf("G: %0.2f rad/s, exceeds threshold of %0.2f m°/s\r\n", gyro, (float)THRESHOLD_GYRO);

			if (telem_monitor & MAG) // Mag
				printf("M: %0.2f mgauss, exceeds threshold of %0.2f mgauss\r\n", mag, (float)THRESHOLD_MAG);
		}
		printf("--------------------------------\n");

		HAL_SuspendTick();
		rtc_flag = 0;
		HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
		HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
		SystemClock_Config();
		HAL_ResumeTick();
		while (!rtc_flag)
			;
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

void Button_Handler(void)
{
	uint32_t timenow = HAL_GetTick();
	if (button_count == 0 || timenow - button_time > 500)
	{
		button_count = 1;
		button_time = timenow;
	}
	else if (button_count == 1)
	{
		button_count = 0;
		if (global_mode)
			global_mode = DEFEND_MODE;
		else
			global_mode = SENTRY_MODE;
		button_time = timenow;
	}
	// printf("button\n");
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
	{
		Error_Handler();
	}

	/** Enable MSI Auto calibration
	 */
	HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	printf("error\n");
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
