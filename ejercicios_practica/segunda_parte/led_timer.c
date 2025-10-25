#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

/*¨
 * Practica simple con timer
 * Se necesita encender y apagar un led en el P0.22 de forma intermitente
 * cada 1s utilizando la interruption del TMR0
 */

#define OUTPUT (uint8_t) 1
#define PIN22 ((uint32_t) (1 << 22))
#define PORT_ZERO (uint8_t) 0


void configPins(void);
void configTimer(void);
void iniciarTimer(void);
void toggleLed(void);

int main(void) {
	configPins();
	configTimer();
	iniciarTimer();
	while(1) {

	}
}

void configPins(void) {
	//config led P0.22
	PINSEL_CFG_Type config_led;
	config_led.Portnum = PINSEL_PORT_0;
	config_led.Pinnum = PINSEL_PIN_22;
	config_led.Funcnum = PINSEL_FUNC_0;
	config_led.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_led.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_led);

	GPIO_SetDir(PORT_ZERO, PIN22, OUTPUT);
	return;
}

void configTimer(void) {
	TIM_TIMERCFG_Type timer_config;
	timer_config.PrescaleOption = TIM_PRESCALE_USVAL;
	timer_config.PrescaleValue = 1000;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_config);

	TIM_MATCHCFG_Type mat0_config;
	mat0_config.MatchChannel = 0;
	mat0_config.IntOnMatch = ENABLE;
	mat0_config.StopOnMatch = DISABLE;
	mat0_config.ResetOnMatch = ENABLE;
	mat0_config.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	mat0_config.MatchValue = 999;
	TIM_ConfigMatch(LPC_TIM0, &mat0_config);
}

void iniciarTimer(void) {
	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
	return;
}

void TIMER0_IRQHandler(void) {
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
		toggleLed();
	}
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	return;
}

void toggleLed(void) {
	static uint8_t toggler = 0;
	toggler++;
	if (toggler % 2) {
		GPIO_SetValue(PORT_ZERO, PIN22);
	} else {
		GPIO_ClearValue(PORT_ZERO, PIN22);
	}
	return;
}
