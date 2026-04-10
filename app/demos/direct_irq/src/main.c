#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include "ezh_app.h"

static int monkey_handler(const struct shell *shell, 
                      size_t argc,
                      char **argv)
{
   ARG_UNUSED(argc);
   ARG_UNUSED(argv);

   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"\r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"                 ██████████████████████████            \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"               ██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██          \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"               ██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"             ██▒▒▒▒░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░██        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"         ██████▒▒░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░██      \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"       ██░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░██████  \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"       ██░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░██░░░░██\r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"       ██░░░░░░▒▒░░░░░░░░░░██░░░░░░░░██░░░░░░░░██░░░░██\r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"         ████░░▒▒░░░░░░░░░░██░░░░░░░░██░░░░░░░░██████  \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"             ██▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░██      \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"     ████      ██▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░██        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"   ██    ██      ██▒▒░░░░░░░░░░░░░░░░░░░░░░██          \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"   ██  ██      ██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"   ██          ██▒▒▒▒▒▒▒▒░░░░░░░░░░░░▒▒▒▒▒▒▒▒██        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"     ████    ██▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒██      \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"         ██████▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒██      \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"             ██▒▒▒▒██▒▒░░░░░░░░░░░░░░░░▒▒██▒▒▒▒██      \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"               ██████▒▒▒▒░░░░░░░░░░░░▒▒▒▒██████        \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"                   ██▒▒▒▒▒▒████████▒▒▒▒▒▒██            \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"                   ██░░░░██        ██░░░░██            \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"                   ██████            ██████            \r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_YELLOW,"\r\n");
   shell_fprintf(shell,SHELL_VT100_COLOR_CYAN,"                          I'm Super\r\n");
      
   return 0;
}

SHELL_CMD_REGISTER(monkey, NULL, "I'm super.", monkey_handler);

LOG_MODULE_REGISTER(main);


int main(void)
{
    LOG_INF("Hello Zephyr!");
    ezh__start_app();

    while (1)
    {   
        LOG_INF("Hello from main!");
        k_sleep(K_MSEC(500));
    }
    
	return 0;
}

