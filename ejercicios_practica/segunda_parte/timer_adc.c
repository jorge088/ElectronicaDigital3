
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpio.h"
/*
 Ejemplo de TIMER, ADC
 Se desea tomar la temperatura
 Si la temp es > a 80° encender un led rojo
 Si la temp esta entre 50° y 80° encender un led amarillo
 Si la temp es menor a 50° encender un led verde
 (Sacado de clase teorica ADC2020)
 */

#define RED_LED ( (uint32_t) (1 << 18) )
#define YELLOW_LED ( (uint32_t) (1 << 17) )
#define GREEN_LED ( (uint32_t) (1 << 16) )
#define ADC_MAX_FREC 200000
#define PORT_ZERO ( (uint8_t) 0 )
#define OUTPUT ( (uint8_t) 1 )
#define CHANNEL_ZERO ( (uint8_t) 0 )
#define SECOND (uint32_t) 10000 // 1s tiene 10000x100e-6s
#define COUNT_CONVERTION ( (uint8_t) 10 )

/*
 * Led rojo P0.18
 * LED AMARILLO P0.17
 * LED VERDE P0.16
 * ANALOG INPUT P0.23
 */

void configPins(void);
void configTimer(void);
void configAdc(void);
void iniciarSistema(void);
void iniciarTimer0(void);
void analizarResultado(uint32_t valorMedido);
void ADC_IRQHandler(void);
void TIMER0_IRQHandler(void);

int main(void) {

	configPins();
	configAdc();
	configTimer();
	iniciarTimer0();
	while(1) {

	}
	return 0;
}

void configPins(void) {
	//Entrada analogica P0.23
	PINSEL_CFG_Type entrada_analogica;
	entrada_analogica.Portnum = PINSEL_PORT_0;
	entrada_analogica.Pinnum = PINSEL_PIN_23;
	entrada_analogica.Funcnum = PINSEL_FUNC_1;
	entrada_analogica.Pinmode = PINSEL_PINMODE_PULLUP;
	entrada_analogica.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&entrada_analogica);

	//Led rojo P0.18

	PINSEL_CFG_Type led_rojo;
	led_rojo.Portnum = PINSEL_PORT_0;
	led_rojo.Pinnum = PINSEL_PIN_18;
	led_rojo.Funcnum = PINSEL_FUNC_0;
	led_rojo.Pinmode = PINSEL_PINMODE_PULLUP;
	led_rojo.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&led_rojo);

	//led amarillo P0.17
	PINSEL_CFG_Type led_amarillo;
	led_amarillo.Portnum = PINSEL_PORT_0;
	led_amarillo.Pinnum = PINSEL_PIN_17;
	led_amarillo.Funcnum = PINSEL_FUNC_0;
	led_amarillo.Pinmode = PINSEL_PINMODE_PULLUP;
	led_amarillo.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&led_amarillo);

	//led verde P0.16
	PINSEL_CFG_Type led_verde;
	led_verde.Portnum = PINSEL_PORT_0;
	led_verde.Pinnum = PINSEL_PIN_16;
	led_verde.Funcnum = PINSEL_FUNC_0;
	led_verde.Pinmode = PINSEL_PINMODE_PULLUP;
	led_verde.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&led_verde);

	GPIO_SetDir(PORT_ZERO, RED_LED, OUTPUT);
	GPIO_SetDir(PORT_ZERO, YELLOW_LED, OUTPUT);
	GPIO_SetDir(PORT_ZERO, GREEN_LED, OUTPUT);
}

void configADC() {
	ADC_Init(LPC_ADC, ADC_MAX_FREC);
	ADC_ChannelCmd(LPC_ADC, CHANNEL_ZERO, ENABLE);
	ADC_IntConfig(LPC_ADC, CHANNEL_ZERO, ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
}

void configTimer() {
	//Timer config
	TIM_TIMERCFG_Type timer_0_config;
	timer_0_config.PrescaleOption = TIM_PRESCALE_USVAL;
	timer_0_config.PrescaleValue = (uint32_t) 100; // cada 100uS aumenta el tc
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_0_config);

	//Match config
	TIM_MATCHCFG_Type match_0_config;
	match_0_config.MatchChannel = 0;
	match_0_config.IntOnMatch = ENABLE;
	match_0_config.ResetOnMatch = ENABLE;
	match_0_config.StopOnMatch = DISABLE;
	match_0_config.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	match_0_config.MatchValue = (30*SECOND); // alcanza match value cada 30s
	TIM_ConfigMatch(LPC_TIM0, &match_0_config);
}

void iniciarTimer0(void) {
	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void) {
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
		// Iniciar conversión ADC cada 30s
		ADC_StartCmd(LPC_ADC, ADC_START_NOW);
	}
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void ADC_IRQHandler(void) {
	static uint32_t resultado_conversion = 0;
	static uint8_t samples_number = 0;

	if (ADC_ChannelGetStatus(LPC_ADC, 0, ADC_DATA_DONE)) {
		resultado_conversion += ADC_ChannelGetData(LPC_ADC, CHANNEL_ZERO);
		samples_number++;
		if (samples_number < COUNT_CONVETION) {
				ADC_StartCmd(LPC_ADC, ADC_START_NOW);
		} else {
			resultado_conversion = resultado_conversion/10;
			//Convertir a temperatura (Supongo usar el LM35, 3.3V , 12bits)
			float volt = (resultado_conversion * 3.3f) / 4095.0f;
			float temp = volt * 100.0f; // 10 mV/°C
			analizarResultado(resultado_conversion);
			//reset valores
			resultado_conversion = 0;
			samples_number = 0;

			ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, DISABLE);
			NVIC_DisableIRQ(ADC_IRQn);
		}
	}
}




void analizarResultado(uint32_t valorMedido) {
	GPIO_ClearValue(PORT_ZERO, RED_LED);
	GPIO_ClearValue(PORT_ZERO, YELLOW_LED);
	GPIO_ClearValue(PORT_ZERO, GREEN_LED);
	if (valorMedido >= 80) {
		GPIO_SetValue(PORT_ZERO, RED_LED);
	} else if (valorMedido >= 50 && valorMedido < 80) {
		GPIO_SetValue(PORT_ZERO, YELLOW_LED);
	} else {
		GPIO_SetValue(PORT_ZERO, GREEN_LED);
	}
}


