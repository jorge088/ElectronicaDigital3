/*
 * Calcular cual es el tiempo máximo que se puede temporizar utilizando
un timer en modo match con máximo valor de prescaler y máximo
divisor de frecuencia de periférico. Especificar el valor a cargar en los
correspondientes registros del timer. Suponer una frecuencia de core
cclk de 50 MHz.
 */

/*
 * Con un cclk de 50MHz, los posibles divisores del cclk para el pcl son 1, 2 y 4.
 * El mayor divisor es 4, este es el que se toma.
 * El TC y MRx son registros de 32bits y los maximos valores que permiten son
 * 0xFFFFFFFF (4294967295)
 * Teniendo en cuenta que T = (PR + 1) * (MR + 1) / PCLK
 *
 * Para maximizar el tiempo hay que elegir
 * PCLK = CCLK / 4
 * PR = 0xFFFFFFFF == 4 294 967 295
 * MR = 0xFFFFFFFF == 4 294 967 295
 *
 * Usandolo en la formula da un tiempo de:
 * T≈2.95×e12 segundos que es un aprox 93,636 años
 *
 */

#include "LPC17xx.h"
#include "lpc17xx_timer.h"

//config del timer

void configTimer(void) {
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_4);
	TIM_TIMERCFG_Type timer_config;
	timer_config.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer_config.PrescaleValue = 0xFFFFFFFF;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_config);

	TIM_MATCHCFG_Type mat0_config;
	mat0_config.MatchChannel = 0;
	mat0_config.IntOnMatch = ENABLE;
	mat0_config.ResetOnMatch = DISABLE;
	mat0_config.StopOnMatch = ENABLE;
	mat0_config.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	mat0_config.MatchValue = 0xFFFFFFFF;
	TIM_ConfigMatch(LPC_TIM0, &mat0_config);

	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void) {
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
		//hacer algo despues de 93años
	}
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}
