#include "stdint.h"


#pragma once

#ifndef _EZH_APP_H
#define _EZH_APP_H


typedef struct _EZHPWM_Para
{
    void  *coprocessor_stack;
    uint32_t *p_buffer;
} EZHPWM_Para;


void ezh_app(void);

void ezh__start_app();

void ezh__send_cmd(uint32_t cmd);

#endif

