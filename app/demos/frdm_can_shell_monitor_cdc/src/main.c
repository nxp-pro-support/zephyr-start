/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * FRDM CAN shell monitor over USB CDC ACM.
 *
 * The application code intentionally stays minimal. CAN inspection,
 * filtering, sending, and receiving are exposed by CONFIG_CAN_SHELL.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("FRDM CAN shell monitor ready. Use `help`, `device list`, and `can`.\n");

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
