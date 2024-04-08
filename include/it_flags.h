#ifndef __IT_FLAGS_H
#define __IT_FLAGS_H

volatile uint8_t sensor_counter;
volatile uint8_t button_count;
volatile uint8_t rtc_flag;
volatile uint32_t button_time;

typedef enum
{
	DEFEND_MODE = 0,
	SENTRY_MODE = 1
} BUG_Mode;

enum Sensors
{
	TEMP     = 0b00000001,
	PRESSURE = 0b00000010,
	HUMIDITY = 0b00000100,
	ACCEL    = 0b00001000,
	GYRO     = 0b00010000,
	MAG      = 0b00100000
};

volatile BUG_Mode global_mode;
volatile uint8_t telem_monitor, sensor_callbacks;

#endif
