/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "zephyr/kernel.h"

int main(void)
{

	uint32_t ticks = 0;

	while(1)
	{
		printk("Hello RTT! %d\n", ticks++);
		k_sleep(K_MSEC(1000));

	}

	return 0;
}
