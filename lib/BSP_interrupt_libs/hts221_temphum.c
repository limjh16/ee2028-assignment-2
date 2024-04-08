#include "hts221_temphum.h"
#include <stdio.h>

static uint8_t TH_Buffer[4];
static uint8_t calib_buffer_1[4], calib_buffer_2[3], calib_buffer_3[6];
static int16_t H0_T0_out, H1_T0_out, H_T_out, H0_rh, H1_rh, T0_out, T1_out, T_out, T0_degC_x8_u16, T1_degC_x8_u16, T0_degC, T1_degC;
// For some reason, one-shot mode worked for 5 minutes across many resets, but never again after that
// From testing, 1Hz mode triggers a conversion almost immediately anyways.
// Benefit over one-shot is, only 1 register needs to be written to when requesting for a new conversion instead of 2.
uint8_t hts221_start_registers[3] = {
	HTS221_TH_ACTIVE_MODE | HTS221_TH_BDU | HTS221_TH_1HZ,
	0, HTS221_TH_DRDY_EN};
uint8_t hts221_register_1_power_down = HTS221_TH_BDU | HTS221_TH_1HZ;

enum Sensors
{
	TEMP     = 0b00000001,
	PRESSURE = 0b00000010,
	HUMIDITY = 0b00000100,
	ACCEL    = 0b00001000,
	GYRO     = 0b00010000,
	MAG      = 0b00100000
};

/**
 * @brief  Initializes peripherals used by the I2C Temperature Sensor driver.
 * @retval TSENSOR status
 */
TSENSOR_Status_TypDef TH_Init_IntMode(I2C_HandleTypeDef *hi2c)
{
	uint8_t read_value = 0;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
		hi2c, HTS221_I2C_ADDRESS,
		HTS221_WHO_AM_I_REG,
		I2C_MEMADD_SIZE_8BIT, &read_value, 1, I2C_DEFAULT_TIMEOUT);
	if (read_value != HTS221_WHO_AM_I_VAL)
	{
		printf("Error! status: %d, whoami value: %d\n", status, read_value);
		return TSENSOR_ERROR;
	}
	// Disable DRDY pin during initialisation to prevent interrupts from messing with this
	HAL_GPIO_DeInit(HTS221_DRDY_EXTI15_GPIO_Port, HTS221_DRDY_EXTI15_Pin);

	HAL_I2C_Mem_Write(
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_CTRL_REG1 | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, hts221_start_registers, 3, I2C_DEFAULT_TIMEOUT);

	// Retrieving calibration data
	HAL_I2C_Mem_Read(
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_H0_RH_X2 | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, calib_buffer_1, 4, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Read(
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_T0_T1_DEGC_H2 | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, calib_buffer_2, 3, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Read(
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_H1_T0_OUT_L | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, calib_buffer_3, 6, I2C_DEFAULT_TIMEOUT);

	H0_rh = calib_buffer_1[0] >> 1;
	H1_rh = calib_buffer_1[1] >> 1;
	H0_T0_out = (((uint16_t)calib_buffer_2[2]) << 8) | (uint16_t)calib_buffer_2[1];
	H1_T0_out = (((uint16_t)calib_buffer_3[1]) << 8) | (uint16_t)calib_buffer_3[0];

	T0_degC_x8_u16 = (((uint16_t)(calib_buffer_2[0] & 0x03)) << 8) | ((uint16_t)calib_buffer_1[2]);
	T1_degC_x8_u16 = (((uint16_t)(calib_buffer_2[0] & 0x0C)) << 6) | ((uint16_t)calib_buffer_1[3]);
	T0_degC = T0_degC_x8_u16 >> 3;
	T1_degC = T1_degC_x8_u16 >> 3;
	T0_out = (((uint16_t)calib_buffer_3[3]) << 8) | (uint16_t)calib_buffer_3[2];
	T1_out = (((uint16_t)calib_buffer_3[5]) << 8) | (uint16_t)calib_buffer_3[4];
	HAL_I2C_Mem_Read( // Clear DRDY bit if any, this has to be a BLOCKING read
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_HR_OUT_L_REG | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, TH_Buffer, 4, I2C_DEFAULT_TIMEOUT);

	TH_Power_Down(hi2c);

	// Initialise DRDY pin + interrupt AFTER sensor is initialised
	__HAL_RCC_GPIOD_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = HTS221_DRDY_EXTI15_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(HTS221_DRDY_EXTI15_GPIO_Port, &GPIO_InitStruct);

	printf("HTS221 Initialised\n");
	HAL_Delay(3);

	return TSENSOR_OK;
}

float T_Process(volatile uint8_t *telem_monitor, float threshold)
{
	float tmp_f;
	T_out = MSBLSB(TH_Buffer, 1);
	tmp_f = (float)(T_out - T0_out) * (float)(T1_degC - T0_degC) / (float)(T1_out - T0_out) + T0_degC;
	if (tmp_f > threshold)
		*telem_monitor |= TEMP;
	return tmp_f;
}

float H_Process(volatile uint8_t *telem_monitor, float threshold)
{
	float tmp_f;
	H_T_out = MSBLSB(TH_Buffer, 0);
	tmp_f = (float)(H_T_out - H0_T0_out) * (float)(H1_rh - H0_rh) / (float)(H1_T0_out - H0_T0_out) + H0_rh;
	// tmp_f = (tmp_f > 100.0f) ? 100.0f
	// 		: (tmp_f < 0.0f) ? 0.0f
	// 						 : tmp_f;
	if (tmp_f > threshold)
		*telem_monitor |= HUMIDITY;
	return tmp_f;
}

HAL_StatusTypeDef TH_Req(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, HTS221_I2C_ADDRESS,
		HTS221_CTRL_REG1,
		I2C_MEMADD_SIZE_8BIT, hts221_start_registers, 1);
	if (status != HAL_OK)
		printf("I2C/DMA Write Error\n");
	return status;
}

HAL_StatusTypeDef TH_Int_Callback(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
	;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(
		hi2c, HTS221_I2C_ADDRESS,
		(HTS221_HR_OUT_L_REG | HTS221_TH_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, TH_Buffer, 4);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}

HAL_StatusTypeDef TH_Power_Down(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, HTS221_I2C_ADDRESS,
		HTS221_CTRL_REG1,
		I2C_MEMADD_SIZE_8BIT, &hts221_register_1_power_down, 1);
	if (status != HAL_OK)
		printf("I2C/DMA Write Error\n");
	return status;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
