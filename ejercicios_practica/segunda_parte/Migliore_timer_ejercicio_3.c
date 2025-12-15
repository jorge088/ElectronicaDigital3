/*
 * Escribir un programa para que por cada presión de un pulsador, la
frecuencia de parpadeo de un led disminuya a la mitad debido a la
modificación del prescaler del Timer 2. El pulsador debe producir una
interrupción por EINT1 con flanco descendente.
 */

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"

#define PULSADOR ((uint32_t) (1 << 11))
#define INPUT ((uint8_t) 0)
#define PORT_TWO ((uint8_t) 2)
#define SECOND 10000
#define prescale_1seg (uint32_t) 100
#define prescale_500ms (uint32_t) 50

void configPins(void);
void configTimer(uint32_t prescale);
void configExtInt(void);

int main(void) {
	configPins();
	configExtInt();
	configTimer(prescale_1seg);
	while(1) {

	}
	return;
}

void configPins(void) {
	//Led P0.6
	PINSEL_CFG_Type configLed;
	configLed.Portnum = PINSEL_PORT_0;
	configLed.Pinnum = PINSEL_PIN_6;
	configLed.Funcnum = PINSEL_FUNC_3;
	configLed.Pinmode = PINSEL_PINMODE_TRISTATE;
	configLed.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&configLed);


	//Pin P2.11 Pulsador pull down
	PINSEL_CFG_Type configPulsador;
	configPulsador.Portnum = PINSEL_PORT_2;
	configPulsador.Pinnum = PINSEL_PIN_11;
	configPulsador.Funcnum = PINSEL_FUNC_1;
	configPulsador.Pinmode = PINSEL_PINMODE_PULLUP;
	configPulsador.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&configPulsador);
	GPIO_SetDir(PORT_TWO, PULSADOR, INPUT);

}

void configExtInt(void) {
	 EXTI_InitTypeDef config_eint;
	  config_eint.EXTI_Line = EXTI_EINT1;
	  config_eint.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE; // flancos
	  config_eint.EXTI_polarity =
	      EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE; // bajada
	  EXTI_Init();
	  EXTI_Config(&config_eint);
	  NVIC_EnableIRQ(EINT1_IRQn);
}

void configTimer(uint32_t prescale) {
	TIM_Cmd(LPC_TIM2, DISABLE);
	TIM_DeInit(LPC_TIM2); // Limpia el timer antes de reconfigurarlo

	TIM_TIMERCFG_Type config_timer2;
	config_timer2.PrescaleOption = TIM_PRESCALE_USVAL;
	config_timer2.PrescaleValue = prescale;
	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &config_timer2);

	TIM_MATCHCFG_Type config_match;
	config_match.MatchChannel = 0;
	config_match.IntOnMatch = DISABLE;
	config_match.StopOnMatch = DISABLE;
	config_match.ResetOnMatch = ENABLE;
	config_match.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	config_match.MatchValue = SECOND;
	TIM_ConfigMatch(LPC_TIM2, &config_match);

	TIM_Cmd(LPC_TIM2, ENABLE);
}

void EINT1_IRQHandler(void) {
	static int slowMode = 0;
	slowMode++;
	if (slowMode % 2) {
		configTimer(prescale_500ms);
	} else {
		configTimer(prescale_1seg);
	}
	EXTI_ClearEXTIFlag(EXTI_EINT1); // limpio bandera
}
