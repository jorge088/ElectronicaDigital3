/*
 * Configurar 4 canales del ADC para que funcionando en modo burst se
obtenga una frecuencia de muestreo en cada uno de 50Kmuestras/seg.
Suponer un CCLK = 100 MHz.
 */

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_adc.h"


void configPins(void);
void configADC(void);

int main(void) {
	configPins();
	configADC();
	while(1) {
		// En modo burst, el ADC convierte continuamente
		uint16_t val0 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0);
		uint16_t val1 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_1);
		uint16_t val2 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_2);
		uint16_t val3 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_3);
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

		//P0.25 AD0.2
		PINSEL_CFG_Type config_pin_adc2;
		config_pin_adc2.Portnum = PINSEL_PORT_0;
		config_pin_adc2.Funcnum = PINSEL_FUNC_1;
		config_pin_adc2.Pinnum = PINSEL_PIN_25;
		config_pin_adc2.Pinmode = PINSEL_PINMODE_TRISTATE;
		config_pin_adc2.OpenDrain = PINSEL_PINMODE_NORMAL;
		PINSEL_ConfigPin(&config_pin_adc2);

		//P0.26 AD0.3
		PINSEL_CFG_Type config_pin_adc3;
		config_pin_adc3.Portnum = PINSEL_PORT_0;
		config_pin_adc3.Funcnum = PINSEL_FUNC_1;
		config_pin_adc3.Pinnum = PINSEL_PIN_26;
		config_pin_adc3.Pinmode = PINSEL_PINMODE_TRISTATE;
		config_pin_adc3.OpenDrain = PINSEL_PINMODE_NORMAL;
		PINSEL_ConfigPin(&config_pin_adc3);
}


void configADC(void) {

	// PCLK_ADC = CCLK/4 = 25 MHz
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_ADC, CLKPWR_PCLKSEL_CCLK_DIV_4);

	/*
	 * CClk = 100MhZ
	 * PCLK = 100MHz / 4 = 25MHz
	 * Con un adc clock max de 13MHz y cada conversión lleva 65 ciclos del adc clock
	 * Fmuestreo => 50KHz => Fmuestreo = 50000 * 65 = 3.25MHz (queda dentro del limite < 13MHz)
	 */
	// Inicializar ADC para 3.25 MHz → 50 ksps por canal × 4 canales
	ADC_Init(LPC_ADC, 3250000);

	// Habilitar canales 0–3
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_1, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_3, ENABLE);

	ADC_BurstCmd(LPC_ADC, ENABLE);

	return;
}
