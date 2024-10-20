#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "rt_config.h"

uint32_t t2_sleep_time;

RT_CONFIG_ITEM(t2_sleep,
              "milliseconds thread t2 should sleep for", 
               &t2_sleep_time, 
               RT_CONFIG_DATA_TYPE_UINT32, 
               sizeof(t2_sleep_time),
               "10",  //Min
               "5000", //max
               "500"); //Default


// Thread configurations
#define CONFIG__T2_TH_STACK_SIZE 2048
#define CONFIG__T2_TH_PRIORITY 5

void t2_thread_task(void *, void *, void *);

K_THREAD_DEFINE(t2_thread, CONFIG__T2_TH_STACK_SIZE,
                t2_thread_task, NULL, NULL, NULL,
                CONFIG__T2_TH_PRIORITY, 0, 0);

LOG_MODULE_REGISTER(t2);


// GPIO configurations
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


void t2_thread_task(void *, void *, void *)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    while (1)
    {

        gpio_pin_toggle_dt(&led);

        k_sleep(K_MSEC(t2_sleep_time));
    }
}



