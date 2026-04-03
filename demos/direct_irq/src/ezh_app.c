#include <zephyr/irq.h>

#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_reset.h"
#include "ezh_init.h"
#include <ezh_app.h>
#include "bunny_build.h"

static uint32_t ezh_stack[16];
static EZHPWM_Para ezh_parameters;
static uint32_t ezh_debug_params[5];
static uint32_t my_ezh_program[128];
uint32_t test_val;

#define EZH_OP_1 24U

void ezh_app__toggle_test_io(void)
{
	E_NOP;
	E_NOP;
	E_PER_READ(R6, ARM2EZH);
	E_LSR(R6, R6, 2);
	E_LSL(R6, R6, 2);
	E_LDR(SP, R6, 0);
	E_LDR(R7, R6, 1);

	E_BSET_IMM(GPO, GPO, EZH_OP_1);
	E_BSET_IMM(GPD, GPD, EZH_OP_1);

	E_HEART_RYTHM_IMM(37493);

E_LABEL("toggle");

	E_BSET_IMM(GPO, GPO, EZH_OP_1);

	E_WAIT_FOR_BEAT();
	E_WAIT_FOR_BEAT();

	E_BCLR_IMM(GPO, GPO, EZH_OP_1);

	E_ADD_IMM(R0, R0, 1);
	E_PER_WRITE(R0, EZH2ARM);
	E_STR(R7, R0, 0);

	E_WAIT_FOR_BEAT();
	E_WAIT_FOR_BEAT();

	E_GOSUB("toggle");
}

ISR_DIRECT_DECLARE(smartdma_irq_handler)
{
	EZH_SetExternalFlag(1);
	test_val = LPC_EZH_ARCH_B0->EZHB_EZH2ARM;
	EZH_SetExternalFlag(0);
	ISR_DIRECT_PM();

	return 1;
}

void ezh__start_app(void)
{
	EZH_SetExternalFlag(0);

	irq_disable(SMARTDMA_IRQn);
	IRQ_DIRECT_CONNECT(SMARTDMA_IRQn, 0, smartdma_irq_handler, 0);

	CLOCK_EnableClock(kCLOCK_Port2);
	PORT_SetPinMux(PORT2, 4U, kPORT_MuxAlt7);
	RESET_PeripheralReset(kSMART_DMA_RST_SHIFT_RSTn);
	CLOCK_EnableClock(kCLOCK_Smartdma);

	ezh_parameters.coprocessor_stack = (void *)ezh_stack;
	ezh_parameters.p_buffer = ezh_debug_params;

	bunny_build(my_ezh_program, sizeof(my_ezh_program), ezh_app__toggle_test_io);
	EZH_Init(&ezh_parameters);
	EZH_boot(my_ezh_program);

	irq_enable(SMARTDMA_IRQn);
}
