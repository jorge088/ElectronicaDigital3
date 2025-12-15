/*
 * En base al ejemplo 1, modificar la configuración del ADC para
considerar la utilización de dos canales de conversión. Determinar a
que frecuencia se encuentra muestreando cada canal.
 */
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"

void configPins(void);
void configADC(void);
void configTimer(void);

#define SECOND 10000

int main(void) {
	configPins();
	configADC();
	configTimer();
	while(1) {

	}
	return 0;
}

void configPins(void) {
	//P0.23 AD0.0
	PINSEL_CFG_Type config_pin_adc0;
	config_pin_adc0.Portnum = PINSEL_PORT_0;
	config_pin_adc0.Funcnum = PINSEL_FUNC_1;
	config_pin_adc0.Pinnum = PINSEL_PIN_23;
	config_pin_adc0.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_pin_adc0.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_pin_adc0);

	//P0.24 AD0.1

	PINSEL_CFG_Type config_pin_adc1;
	config_pin_adc1.Portnum = PINSEL_PORT_0;
	config_pin_adc1.Funcnum = PINSEL_FUNC_1;
	config_pin_adc1.Pinnum = PINSEL_PIN_24;
	config_pin_adc1.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_pin_adc1.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_pin_adc1);

	//P1.29
	PINSEL_CFG_Type config_pin_mat1;
	config_pin_mat1.Portnum = PINSEL_PORT_1;
	config_pin_mat1.Funcnum = PINSEL_FUNC_3;
	config_pin_mat1.Pinnum = PINSEL_PIN_29;
	config_pin_mat1.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_pin_mat1.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_pin_mat1);
}

void configADC(void) {
	ADC_Init(LPC_ADC, 32000); //16kHz = 16000 => Niquist = 32000

	ADC_BurstCmd(LPC_ADC, DISABLE);

	// Iniciar conversión con el evento del Timer0 MAT0.1
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING);

	//Habilita channel 0 y 1
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_1, ENABLE);

	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN1, ENABLE);

	NVIC_EnableIRQ(ADC_IRQn);
	return;
}

void configTimer(void) {
	TIM_TIMERCFG_Type config_timer;
	config_timer.PrescaleOption = TIM_PRESCALE_USVAL;
	config_timer.PrescaleValue = 50;
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &config_timer);

	TIM_MATCHCFG_Type config_mat1;
	config_mat1.MatchChannel = 1;
	config_mat1.IntOnMatch = DISABLE;
	config_mat1.StopOnMatch = DISABLE;
	config_mat1.ResetOnMatch = ENABLE;
	config_mat1.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	config_mat1.MatchValue = SECOND;
	TIM_ConfigMatch(LPC_TIM0, &config_mat1);

	TIM_Cmd(LPC_TIM0, ENABLE);
}

void ADC_IRQHandler(void) {
	uint16_t val0 = 0;
	uint16_t val1 = 0;
	if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE)) {
		val0 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0);
	}
	if (ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_1, ADC_DATA_DONE)) {
		val1 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_1);
	}
	return;
}

/*
 * Frecuencia total de muestreo 32kHz
 * Fcanal = Ftotal /Ncanales = 32000 / 2 = 16000Hz
 * Con 2 canales cada uno tiene un f de muestreo de 16kHz
 */
