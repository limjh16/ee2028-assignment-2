#include "lps22hb_baro.h"
#include <stdio.h>

static uint8_t P_Buffer[3];

/**
 * @brief  Initializes peripherals used by the I2C Pressure Sensor driver.
 * @retval PSENSOR status
 */
PSENSOR_Status_TypDef BARO_PSENSOR_Init(I2C_HandleTypeDef *hi2c)
{
	uint8_t read_value = 0;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
		hi2c, LPS22HB_I2C_ADDRESS,
		LPS22HB_WHO_AM_I_REG,
		I2C_MEMADD_SIZE_8BIT, &read_value, 1, I2C_DEFAULT_TIMEOUT);
	if (read_value != LPS22HB_WHO_AM_I_VAL)
	{
		printf("Error! status: %d, whoami value: %d\n", status, read_value);
		return PSENSOR_ERROR;
	}
	// Disable DRDY pin during initialisation to prevent interrupts from messing with this
	HAL_GPIO_DeInit(LPS22HB_INT_DRDY_EXTI0_GPIO_Port, LPS22HB_INT_DRDY_EXTI0_Pin);

	uint8_t lps22hb_registers[3];
	lps22hb_registers[0] = LPS22HB_BARO_POWER_DOWN_MODE | LPS22HB_BARO_BDU;
	lps22hb_registers[1] = LPS22HB_BARO_AUTO_INCR_EN | LPS22HB_BARO_ONESHOT_EN;
	lps22hb_registers[2] = LPS22HB_BARO_INT_DRDY_EN;
	HAL_I2C_Mem_Write(
		hi2c, LPS22HB_I2C_ADDRESS,
		LPS22HB_CTRL_REG1,
		I2C_MEMADD_SIZE_8BIT, lps22hb_registers, 3, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Read( // Clear DRDY bit if any, this has to be a BLOCKING read
		hi2c, LPS22HB_I2C_ADDRESS,
		LPS22HB_PRESS_OUT_XL_REG,
		I2C_MEMADD_SIZE_8BIT, P_Buffer, 3, I2C_DEFAULT_TIMEOUT);

	// Initialise DRDY pin + interrupt AFTER sensor is initialised
	LPS22HB_DRDY_GPIO_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LPS22HB_INT_DRDY_EXTI0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LPS22HB_INT_DRDY_EXTI0_GPIO_Port, &GPIO_InitStruct);
	
	printf("LPS22HB Initialised\n");
	HAL_Delay(3);

	return PSENSOR_OK;
}

/**
 * @brief Processes pressure values in the buffer
 * 
 * @return (float) Pressure in mbar
 */
float BARO_PSENSOR_Process(volatile uint8_t *telem_monitor, float threshold)
{
	uint32_t tmp = 0;
	/* Build the raw data */
	for (int i = 0; i < 3; i++)
		tmp |= (((uint32_t)P_Buffer[i]) << (8 * i));
	/* convert the 2's complement 24 bit to 2's complement 32 bit */
	if (tmp & 0x00800000)
		tmp |= 0xFF000000;
	float pressure = (int32_t)tmp / 4096.0f;
	if (pressure > threshold)
		*telem_monitor |= 0b00000010;
	return pressure;
}

/**
 * @brief Requests pressure sensor to capture a new reading
 * 
 * @param hi2c I2C handler
 * @return (HAL_StatusTypeDef) HAL status 
 */
HAL_StatusTypeDef BARO_PSENSOR_Req(I2C_HandleTypeDef *hi2c)
{
	uint8_t lps22hb_register2 = LPS22HB_BARO_AUTO_INCR_EN | LPS22HB_BARO_ONESHOT_EN;
	while (hi2c->State != HAL_I2C_STATE_READY);
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, LPS22HB_I2C_ADDRESS,
		LPS22HB_CTRL_REG2,
		I2C_MEMADD_SIZE_8BIT, &lps22hb_register2, 1);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}

HAL_StatusTypeDef BARO_PSENSOR_Int_Callback(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(
		hi2c, LPS22HB_I2C_ADDRESS,
		LPS22HB_PRESS_OUT_XL_REG,
		I2C_MEMADD_SIZE_8BIT, P_Buffer, 3);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
