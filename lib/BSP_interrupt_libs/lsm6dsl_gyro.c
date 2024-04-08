#include "lsm6dsl_gyro.h"
#include <stdio.h>
#include <math.h>

static uint8_t LSM6DSL_Buffer[12];

enum Sensors
{
	TEMP     = 0b00000001,
	PRESSURE = 0b00000010,
	HUMIDITY = 0b00000100,
	ACCEL    = 0b00001000,
	GYRO     = 0b00010000,
	MAG      = 0b00100000
};

uint8_t lsm6dsl_registers_sleep[4] = {
	LSM6DSL_ODR_POWER_DOWN | LSM6DSL_ACC_FULLSCALE_2G, // ACC in pwr down
	LSM6DSL_ODR_208Hz | LSM6DSL_GYRO_FS_245,
	LSM6DSL_BDU_BLOCK_UPDATE | LSM6DSL_ACC_GYRO_IF_INC_ENABLED,// | LSM6DSL_ACC_GYRO_SW_RESET,
	LSM6DSL_GYRO_SLEEP_EN | LSM6DSL_INT2_ON_INT1 | LSM6DSL_DRDY_MASK
};
uint8_t lsm6dsl_registers_wake[4] = {
	LSM6DSL_ODR_208Hz | LSM6DSL_ACC_FULLSCALE_2G,
	LSM6DSL_ODR_208Hz | LSM6DSL_GYRO_FS_245,
	LSM6DSL_BDU_BLOCK_UPDATE | LSM6DSL_ACC_GYRO_IF_INC_ENABLED,// | LSM6DSL_ACC_GYRO_SW_RESET,
	LSM6DSL_INT2_ON_INT1 | LSM6DSL_DRDY_MASK
};
// - Though only registers [0] and [3] are required, sending it in one shot with auto incr
//     is the same number of registers transferred compared to sending it 2 separate times.
// - Makes code cleaner too

/** @defgroup STM32L475E_IOT01_GYROSCOPE_Private_Functions GYROSCOPE Private Functions
 * @{
 */
/**
 * @brief  Initialize Gyroscope.
 * @retval GYRO_OK or GYRO_ERROR
 */
GYRO_StatusTypeDef LSM6DSL_Init_IntMode(I2C_HandleTypeDef *hi2c)
{
	uint8_t read_value = 0;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_WHO_AM_I_REG,
		I2C_MEMADD_SIZE_8BIT, &read_value, 1, I2C_DEFAULT_TIMEOUT);
	if (read_value != LSM6DSL_ACC_GYRO_WHO_AM_I)
	{
		printf("Error! status: %d, whoami value: %d\n", status, read_value);
		return GYRO_ERROR;
	}
	// Disable DRDY pin during initialisation to prevent interrupts from messing with this
	HAL_GPIO_DeInit(LSM6DSL_INT1_EXTI11_GPIO_Port, LSM6DSL_INT1_EXTI11_Pin);

	uint8_t lsm6dsl_int1_ctrl_registers = LSM6DSL_GYRO_DRDY_INT1_EN; // DRDY only gyro
	// FIFO is default in bypass mode, no need to write to it

	HAL_I2C_Mem_Write(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_CTRL1_XL,
		I2C_MEMADD_SIZE_8BIT, lsm6dsl_registers_wake, 4, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Write(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_INT1_CTRL,
		I2C_MEMADD_SIZE_8BIT, &lsm6dsl_int1_ctrl_registers, 1, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Write(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_CTRL1_XL,
		I2C_MEMADD_SIZE_8BIT, lsm6dsl_registers_sleep, 4, I2C_DEFAULT_TIMEOUT);
	HAL_I2C_Mem_Read( // Clear DRDY bit if any, this has to be a BLOCKING read
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_OUTX_L_G,
		I2C_MEMADD_SIZE_8BIT, LSM6DSL_Buffer, 2, I2C_DEFAULT_TIMEOUT);

	// Initialise DRDY pin + interrupt AFTER sensor is initialised
	LSM6DSL_DRDY_GPIO_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LSM6DSL_INT1_EXTI11_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LSM6DSL_INT1_EXTI11_GPIO_Port, &GPIO_InitStruct);

	printf("LSM6DSL Initialised\n");
	HAL_Delay(3);

	return GYRO_OK;
}

/**
 * @brief  Process XYZ angular acceleration from the Gyroscope.
 * @param  pfData: pointer on floating array
 */
void GYRO_ProcessXYZ(float *pfData)
{
	float sensitivity = LSM6DSL_GYRO_SENSITIVITY_245DPS;
	for (int i = 0; i < 3; i++)
	{
		pfData[i] = (float)((int16_t)MSBLSB(LSM6DSL_Buffer, i) *
							sensitivity);
	}
}

float GYRO_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold)
{
	float sensitivity = LSM6DSL_GYRO_SENSITIVITY_245DPS;
	float magnitude = 0;
	for (int i = 0; i < 3; i++)
	{
		float current = ((int16_t)MSBLSB(LSM6DSL_Buffer, i) *
						 sensitivity);
		current *= current;
		magnitude += current;
	}
	magnitude = sqrtf(magnitude);
	if (magnitude > threshold)
		*telem_monitor |= GYRO;
	return magnitude;
}

/**
 * @brief  Process XYZ acceleration values.
 * @param  pDataXYZ Pointer on 3 angular accelerations table with
 *                  pDataXYZ[0] = X axis, pDataXYZ[1] = Y axis, pDataXYZ[2] = Z axis
 * @retval None
 */
void ACCEL_ProcessXYZ(int16_t *pDataXYZ)
{
	float sensitivity = LSM6DSL_ACC_SENSITIVITY_2G;
	for (int i = 3; i < 6; i++)
	{
		pDataXYZ[i - 3] = ((int16_t)MSBLSB(LSM6DSL_Buffer, i) *
						   sensitivity);
	}
}

float ACCEL_ProcessMagnitude(volatile uint8_t *telem_monitor, float threshold)
{
	float sensitivity = LSM6DSL_ACC_SENSITIVITY_2G;
	float magnitude = 0;
	for (int i = 3; i < 6; i++)
	{
		float current = ((int16_t)MSBLSB(LSM6DSL_Buffer, i) *
						 sensitivity);
		current *= current;
		magnitude += current;
	}
	magnitude = sqrtf(magnitude);
	if (magnitude > threshold)
		*telem_monitor |= ACCEL;
	return magnitude;
}

HAL_StatusTypeDef LSM6DSL_Req(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_CTRL1_XL,
		I2C_MEMADD_SIZE_8BIT, lsm6dsl_registers_wake, 4);
	if (status != HAL_OK)
		printf("I2C/DMA Write Error\n");
	return status;
}

HAL_StatusTypeDef LSM6DSL_Int_Callback(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read_DMA(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_OUTX_L_G,
		I2C_MEMADD_SIZE_8BIT, LSM6DSL_Buffer, 12);
	if (status != HAL_OK)
		printf("I2C/DMA Read Error\n");
	return status;
}

HAL_StatusTypeDef LSM6DSL_Power_Down(I2C_HandleTypeDef *hi2c)
{
	while (hi2c->State != HAL_I2C_STATE_READY)
		;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(
		hi2c, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW,
		LSM6DSL_ACC_GYRO_CTRL1_XL,
		I2C_MEMADD_SIZE_8BIT, lsm6dsl_registers_sleep, 4);
	if (status != HAL_OK)
		printf("I2C/DMA Write Error\n");
	return status;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
