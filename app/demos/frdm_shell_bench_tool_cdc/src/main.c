/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * FRDM shell bench tool over USB CDC ACM.
 *
 * The CDC ACM serial backend, I2C commands, and GPIO commands are enabled by
 * Kconfig/devicetree. There is intentionally no application bus logic here.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("FRDM CDC shell bench tool ready. Use `help`, `device list`, `i2c`, and `gpio`.\n");

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
