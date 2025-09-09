
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>


#if CONFIG_BOARD_FRDM_MCXN947 == 1

#define TEMP_SENSOR_I2C_ADDRESS		0x48
#define TEMP_SENSOR_RESOLUTION		0.0625f		//0.0625 °C

static const struct device * temp_sensor_i2c = DEVICE_DT_GET(DT_NODELABEL(flexcomm5_lpi2c5));

static int read_temperature_handler(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t read_values [2];
	int16_t temperature_register_value;
	float temperature_value;
	int error;

	error = i2c_read(temp_sensor_i2c, read_values, 2, TEMP_SENSOR_I2C_ADDRESS);
	
	if(error != 0)
	{
		shell_fprintf(shell,
                       SHELL_VT100_COLOR_RED,
                       "No communication with temperature sensor\n");
		return error;
	}

	shell_fprintf(shell,
                 SHELL_VT100_COLOR_DEFAULT,
                  "\np3t1755 data reg : %02x %02x \n", read_values[0], read_values[1]);
	
	temperature_register_value = ((int16_t)read_values[0] << 8) | (int16_t)read_values[1];
	temperature_value = (temperature_register_value >> 4) * TEMP_SENSOR_RESOLUTION;
	
	shell_fprintf(shell, 
                  SHELL_VT100_COLOR_DEFAULT,
                   "\nTemperature : %.1f °C \n\n", (double)temperature_value);

	return 0; 
}

SHELL_CMD_REGISTER(read_temperature, NULL, "Read the P3T1755DP temperature sensor of the FRDM board", read_temperature_handler);

#endif

LOG_MODULE_REGISTER(I2C_SHELL);

int main(void)
{	

	#if CONFIG_BOARD_FRDM_MCXN947 == 1
		if (!device_is_ready(temp_sensor_i2c))
		{
			LOG_ERR("temp_sensor_i2c device not found!");
			return 1;
		}
	#endif


	while(1)
	{
		k_msleep(50);
	}
	
	
	return 0;
}
