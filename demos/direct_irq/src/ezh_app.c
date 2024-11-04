
#include <zephyr/irq.h>

#include "fsl_common.h"
//#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "ezh_init.h"
#include <ezh_app.h>
#include "bunny_build.h"

#include "fsl_device_registers.h"

uint32_t ezh_stack[16];

EZHPWM_Para ezh_parameters;

uint32_t ezh_debug_params[5];

__attribute((section("SRAM1"))) uint32_t my_ezh_program[256];
__attribute((section("SRAM1"))) uint32_t my_ezh_program2[256];

uint32_t test_val = 0;

//Magic / undocumented IRQ vector for the EZH to ARM
#define   EZH__Reserved46_IRQn             (IRQn_Type) 30

//LPC55 has quick/direct access to GPIO Port 0
//Using PIO_3 for test GPIO. 
#define   EZH_TEST_GPIO                         3        

void ezh_app__toggle_test_io(void)
{
    E_NOP;

    E_NOP;
    E_PER_READ(R6, ARM2EZH); //Read the base address of the parameters into R6

    E_LSR(R6, R6, 2);        //make sure parameter structure 32-bit aligned
    E_LSL(R6, R6, 2);        //

    E_LDR(SP, R6, 0);        //load stack pointer from the 1st 32-bit word of the parameter struct

	E_LDR(R7, R6, 1);        //R7 -> load the base address of debug parameter array into R7

    /* Set EZH_TEST_GPIO  as output , Direction bit = 1 */
    E_BSET_IMM(GPD, GPD, EZH_TEST_GPIO);

    //Make the default setting as high
    E_BSET_IMM(GPO, GPO, EZH_TEST_GPIO);
    
    /* set the 16-bit heartbeat counter period to 250uS assuming 150MHz Clock*/
    E_HEART_RYTHM_IMM((150000000 / 4000));

 E_LABEL("toggle");

 	E_BSET_IMM(GPO, GPO, EZH_TEST_GPIO); //Set test GPIO

	E_WAIT_FOR_BEAT();
	E_WAIT_FOR_BEAT();

    E_BCLR_IMM(GPO, GPO, EZH_TEST_GPIO); //Clear test GPIO

    //Increment our counter and interrupt the ARM
    E_ADD_IMM(R0, R0, 1);

    //Double Speed Counter
    E_ADD_IMM(R1, R1, 2);
    
    E_PER_WRITE(R0, EZH2ARM);

    E_STR(R7,R0,0); //store the counter in the 1st parameter
    E_STR(R7,R1,1); //store the double speed counter in the 2nd parameter


    E_WAIT_FOR_BEAT();
    E_WAIT_FOR_BEAT();

    E_GOSUB("toggle");
}

ISR_DIRECT_DECLARE(Reserved46_IRQHandler__EZH)
{
	EZH_SetExternalFlag(1) ;

	test_val = LPC_EZH_ARCH_B0->EZHB_EZH2ARM;

	EZH_SetExternalFlag(0) ;

    return 0;
}

void ezh__start_app()
{

MRT0->CHANNEL[0].STAT = 0;

    EZH_SetExternalFlag(0) ;

	irq_disable(EZH__Reserved46_IRQn);            // EZH irq NUMBER 30
     

    IRQ_DIRECT_CONNECT(EZH__Reserved46_IRQn, 0, Reserved46_IRQHandler__EZH, 0) //IRQ_ZERO_LATENCY doesn't build yet

    
    //Magic mux setting to use port 0.3 directly in EZH
    IOCON->PIO[0][3]  = PINFUNC_EZH | 2<<4 | 1<<8;


	CLOCK_EnableClock(kCLOCK_Ezhb);        // enable EZH clock

    ezh_parameters.coprocessor_stack = (void *)ezh_stack;
	ezh_parameters.p_buffer =  (uint32_t *)ezh_debug_params;

	EZH_Init(&ezh_parameters);					       //EZH initialisation

	bunny_build(&my_ezh_program[0],
	    	    sizeof(my_ezh_program),
				ezh_app__toggle_test_io
				);

	EZH_boot(my_ezh_program);				     //start EZH

	irq_enable(EZH__Reserved46_IRQn);                //EZH irq number 30

}   
