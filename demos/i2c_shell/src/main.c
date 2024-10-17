/*
 * Copyright (c) 2024 Open Pixel Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#if CONFIG_BOARD_FRDM_MCXN947 == 1

#define TEMP_SENSOR_I2C_ADDRESS		0X48
#define TEMP_SENSOR_RESOLUTION		0.0625f		//0.0625 °C

static const struct device * i2c5_bus = DEVICE_DT_GET(DT_NODELABEL(flexcomm5_lpi2c5));

int extra_clock_init()
{


    CLOCK_SetClkDiv(kCLOCK_DivFlexcom5Clk, 1u); 
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);

    return 0;
}
SYS_INIT(extra_clock_init, PRE_KERNEL_1, 0);


static int read_temperature_handler(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t read_values [2];
	int16_t temperature_register_value;
	float temperature_value;
	int error;

	error = i2c_read(i2c5_bus, read_values, 2, TEMP_SENSOR_I2C_ADDRESS);
	
	if(error != 0)
	{
		shell_fprintf(shell, SHELL_VT100_COLOR_RED, "No communication with temperature sensor\n");
		return -1;
	}

	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "Temperature registers value %02x%02x \n", read_values[0], read_values[1]);
	
	temperature_register_value = ((int16_t)read_values[0] << 8) | (int16_t)read_values[1];
	temperature_value = (temperature_register_value >> 4) * TEMP_SENSOR_RESOLUTION;
	
	shell_fprintf(shell, SHELL_VT100_COLOR_DEFAULT, "Temperature value %.1f \n", (double)temperature_value);

	return 0; 
}

SHELL_CMD_REGISTER(read_temperature, NULL, "Read the P3T1755DP temperature sensor of the FRDM board", read_temperature_handler);

#endif

LOG_MODULE_REGISTER(I2C_SHELL);

int main(void)
{	

	#if CONFIG_BOARD_FRDM_MCXN947 == 1

		if (!device_is_ready(i2c5_bus))
		{
			LOG_ERR("I2C5 device not found!");
			return 1;
		}
	#endif


	while(1)
	{
		k_msleep(50);
	}
	
	
	return 0;
}
