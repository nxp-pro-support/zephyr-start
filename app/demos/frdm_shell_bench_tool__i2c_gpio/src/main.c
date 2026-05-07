/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * FRDM shell bench tool.
 *
 * There is intentionally no I2C or GPIO application logic here. Those
 * behaviors are exposed by enabling Zephyr shell modules in prj.conf.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>



int main(void)
{

	printk("FRDM shell bench tool ready. Use `help`, `device list`, `i2c`, and `gpio`.\n");

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
