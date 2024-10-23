#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "fsl_clock.h"
#include "fsl_port.h"
#include "fsl_common.h"




LOG_MODULE_REGISTER(main);


int main(void)
{
    LOG_INF("Bare-metal-direct application");

    //GPIO 0 10 RED LED
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Port0);


    const port_pin_config_t port0_10_pinB12_config = {/* Internal pull-up/down resistor is disabled */
                                                      kPORT_PullDisable,
                                                      /* Low internal pull resistor value is selected. */
                                                      kPORT_LowPullResistor,
                                                      /* Fast slew rate is configured */
                                                      kPORT_FastSlewRate,
                                                      /* Passive input filter is disabled */
                                                      kPORT_PassiveFilterDisable,
                                                      /* Open drain output is disabled */
                                                      kPORT_OpenDrainDisable,
                                                      /* Low drive strength is configured */
                                                      kPORT_HighDriveStrength,
                                                      /* Pin is configured as PIO0_10 */
                                                      kPORT_MuxAlt0,
                                                      /* Digital input enabled */
                                                      kPORT_InputBufferEnable,
                                                      /* Digital input is not inverted */
                                                      kPORT_InputNormal,
                                                      /* Pin Control Register fields [15:0] are not locked */
                                                      kPORT_UnlockRegister};
    /* PORT0_10 (pin B12) is configured as PIO0_10 */
    PORT_SetPinConfig(PORT0, 10U, &port0_10_pinB12_config);

    gpio_pin_config_t led_red_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1
    };

    GPIO_PinInit(GPIO0, 10, &led_red_config);

    while (1)
    {   
        GPIO_PortToggle(GPIO0, 1u << 10);
        k_sleep(K_MSEC(500));
    }
    
	return 0;
}

