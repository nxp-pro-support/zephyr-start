#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <rt_config.h>

uint32_t main_sleep_time;

RT_CONFIG_ITEM(main_sleep,
              "milliseconds the main thread should sleep for", 
               &main_sleep_time, 
               RT_CONFIG_DATA_TYPE_UINT32, 
               sizeof(main_sleep_time),
               "10",   //Min
               "5000", //max
               "500"); //Default

int main(void)
{
    while (1)
    {   
        k_sleep(K_MSEC(main_sleep_time));
    }
    
	return 0;
}

