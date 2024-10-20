#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <rt_config.h>


int main(void)
{
    printk("Hello Zephyr!");

    while (1)
    {   
       
        k_sleep(K_MSEC(500));
    }
    
	return 0;
}

