#include "lis3mdl_magneto.h"
#include <stdio.h>
#include <math.h>

static uint8_t MAGNETO_Buffer[6];

/**
 * @brief Initialize a magnetometer sensor
 * @retval COMPONENT_ERROR in case of failure
 */
MAGNETO_StatusTypeDef MAGNETO_Init_SingleMode(I2C_HandleTypeDef *hi2c)
{
	uint8_t read_value = 0;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
		hi2c, LIS3MDL_MAG_I2C_ADDRESS_HIGH,
		LIS3MDL_MAG_WHO_AM_I_REG,
		I2C_MEMADD_SIZE_8BIT, &read_value, 1, I2C_DEFAULT_TIMEOUT);
	if (read_value != I_AM_LIS3MDL)
	{
		printf("Error! status: %d, whoami value: %d\n", status, read_value);
		return MAGNETO_ERROR;
	}
	// Disable DRDY pin during initialisation to prevent interrupts from messing with this
	HAL_GPIO_DeInit(LSM3MDL_DRDY_EXTI8_GPIO_Port, LSM3MDL_DRDY_EXTI8_Pin);

	uint8_t lis3mdl_registers[5];
	lis3mdl_registers[0] = LIS3MDL_MAG_TEMPSENSOR_ENABLE | LIS3MDL_MAG_OM_XY_HIGH | LIS3MDL_MAG_ODR_80_HZ;
	lis3mdl_registers[1] = LIS3MDL_MAG_FS_4_GA | LIS3MDL_MAG_REBOOT_DEFAULT | LIS3MDL_MAG_SOFT_RESET_DEFAULT;
	lis3mdl_registers[2] = LIS3MDL_MAG_CONFIG_NORMAL_MODE | LIS3MDL_MAG_SINGLE_MODE;
	lis3mdl_registers[3] = LIS3MDL_MAG_OM_Z_HIGH | LIS3MDL_MAG_BLE_LSB;
	lis3mdl_registers[4] = LIS3MDL_MAG_BDU_MSBLSB;
	HAL_I2C_Mem_Write(
		hi2c, LIS3MDL_MAG_I2C_ADDRESS_HIGH,
		(LIS3MDL_MAG_CTRL_REG1 | LIS3MDL_MAG_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, lis3mdl_registers, 5, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Read( // Clear DRDY bit if any, this has to be a BLOCKING read
		hi2c, LIS3MDL_MAG_I2C_ADDRESS_HIGH,
		(LIS3MDL_MAG_OUTX_L | LIS3MDL_MAG_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, MAGNETO_Buffer, 2, I2C_DEFAULT_TIMEOUT);

	// Initialise DRDY pin + interrupt AFTER sensor is initialised
	MAG_DRDY_GPIO_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LSM3MDL_DRDY_EXTI8_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LSM3MDL_DRDY_EXTI8_GPIO_Port, &GPIO_InitStruct);

	printf("LIS3MDL Initialised\n");
	HAL_Delay(3);

	return MAGNETO_OK;
}

/**
 * @brief  Get XYZ magnetometer values.
 * @param  pData Pointer on 3 magnetometer values table with
 *                  pData[0] = X axis, pData[1] = Y axis, pData[2] = Z axis
 */
void MAGNETO_ProcessXYZ(int16_t *pData)
{
	float sensitivity = LIS3MDL_MAG_SENSITIVITY_FOR_FS_4GA;
	for (uint8_t i = 0; i < 3; i++)
		pData[i] = ((int16_t)((((uint16_t)MAGNETO_Buffer[2 * i + 1]) << 8) + (uint16_t)MAGNETO_Buffer[2 * i]) * sensitivity);
}

float MAGNETO_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold)
{
	float sensitivity = LIS3MDL_MAG_SENSITIVITY_FOR_FS_4GA;
	float magnitude = 0;
	for (uint8_t i = 0; i < 3; i++)
	{
		float current = ((int16_t)((((uint16_t)MAGNETO_Buffer[2 * i + 1]) << 8) + (uint16_t)MAGNETO_Buffer[2 * i]) * sensitivity);
		current *= current;
		magnitude += current;
	}
	magnitude = sqrtf(magnitude);
	if (magnitude > threshold)
		*telem_monitor |= 0b00100000;
	return magnitude;
}

HAL_StatusTypeDef MAGNETO_Req(I2C_HandleTypeDef *hi2c)
{
	uint8_t lis3mdl_register3 = LIS3MDL_MAG_CONFIG_NORMAL_MODE | LIS3MDL_MAG_SINGLE_MODE;
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, LIS3MDL_MAG_I2C_ADDRESS_HIGH,
		LIS3MDL_MAG_CTRL_REG3,
		I2C_MEMADD_SIZE_8BIT, &lis3mdl_register3, 1);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}

HAL_StatusTypeDef MAGNETO_Int_Callback(I2C_HandleTypeDef *hi2c)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(
		hi2c, LIS3MDL_MAG_I2C_ADDRESS_HIGH,
		(LIS3MDL_MAG_OUTX_L | LIS3MDL_MAG_AUTO_INCR),
		I2C_MEMADD_SIZE_8BIT, MAGNETO_Buffer, 6);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}
