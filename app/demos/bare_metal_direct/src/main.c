#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

/*
    From the MCUXpresso SDK
*/
#include "fsl_clock.h"
#include "fsl_device_registers.h"


//Red LED on FRDM-MCX is on GPIO0  Port 10
#define RED_LED_PIN           10
#define RED_LED_GPIO          GPIO0

LOG_MODULE_REGISTER(main);

int main(void)
{
    LOG_INF("Bare-metal-direct application for MCXN947");

    //Enable the clock on GPIO0
    CLOCK_EnableClock(kCLOCK_Gpio0);

    //Set to output mode
    GPIO0->PDDR |= (1 << RED_LED_PIN);

    //Clear the pin
    GPIO0->PCOR = RED_LED_PIN;

    while (1)
    {   
        GPIO0->PTOR |= (1 << RED_LED_PIN);
       
        k_sleep(K_MSEC(500));
    }
    
	return 0;
}


