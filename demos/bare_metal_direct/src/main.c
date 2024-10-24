#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "fsl_clock.h"
#include "fsl_device_registers.h"

#define LED_PIN     10

LOG_MODULE_REGISTER(main);

int main(void)
{
    LOG_INF("Bare-metal-direct application");

    CLOCK_EnableClock(kCLOCK_Gpio0);

    GPIO0->PDDR |= (1 << LED_PIN);

    while (1)
    {   
        GPIO0->PTOR |= (1 << LED_PIN);
        k_sleep(K_MSEC(500));
    }
    
	return 0;
}


