/*
 * Utilizar un canal del ADC en modo burst para leer datos y guardarlos en un buffer en memoria.
 * Se toman 1024 datos. Cada 1 minuto se debe deshabilitar la lectura del adc y habilitar la salida de
 * los datos guardados en el buffer por medio del DAC. A su vez cada 1 min se repetira la interrupción
 * que habilita nuevamente el adc. Utilizar DMA.
 */
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"

#define ADDRESS ((0x2007C000))
#define SECOND 10000

void configPins(void);
void configTimer(void);
void configADC(void);
void configDAC(void);
void config_dma_adc(void);
void config_dma_dac(void);
void iniciar_dma(void);


uint16_t *buffer = (uint16_t*) ADDRESS;

int main(void) {
	GPDMA_LLI_Type adcLLI;
	GPDMA_LLI_Type dacLLI;
	configPins();
	configADC();
	configDAC();
	configTimer();
	iniciar_dma();
	config_dma_adc();
	while(1) {

	}
	return 0;
}


void configPins(void) {
	//0.23 AD0
	PINSEL_CFG_Type config_adc;
	config_adc.Portnum = PINSEL_PORT_0;
	config_adc.Pinnum = PINSEL_PIN_23;
	config_adc.Funcnum = PINSEL_FUNC_1;
	config_adc.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_adc.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_adc);
}

void configDac(void) {
	DAC_CONVERTER_CFG_Type config_dac;
	config_dac.DBLBUF_ENA = ENABLE;
	config_dac.CNT_ENA = ENABLE;
	config_dac.DMA_ENA = ENABLE;
	DAC_Init(LPC_DAC);
	DAC_ConfigDAConverterControl(LPC_DAC, &config_dac);
	DAC_SetDMATimeOut(0xFF); //TODO
}

void configADC(void) {
	ADC_Init(LPC_ADC, 20000);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
	ADC_BurstCmd(LPC_ADC, ENABLE);
}

void configTimer(void) {
	TIM_TIMERCFG_Type config_timer;
	config_timer.PrescaleOption = TIM_PRESCALE_USVAL;
	config_timer.PrescaleValue = 100; //100us
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &config_timer);

	TIM_MATCHCFG_Type config_mat0;
	config_mat0.MatchChannel = 0;
	config_mat0.StopOnMatch = DISABLE;
	config_mat0.IntOnMatch = ENABLE;
	config_mat0.ResetOnMatch = ENABLE;
	config_mat0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	config_mat0.MatchValue = 60*SECOND; //1min

	TIM_ConfigMatch(LPC_TIM0, &config_mat0);

	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void config_dma_adc(void) {

	adcLLI.SrcAddr = LPC_ADC->ADGDR;

	adcLLI.DstAddr = buffer;
	adcLLI.NextLLI = &adcLLI;
	adcLLI.Control =
			1024 |
			(1 << 12) |
			(1 << 15) |
			(1 << 18) |
			(1 << 21);


	GPDMA_Channel_CFG_Type config_dma_adc;
	config_dma_adc.ChannelNum = 0;
	config_dma_adc.TransferSize = 1024;
	config_dma_adc.SrcMemAddr = LPC_ADC->ADGDR;
	config_dma_adc.DstMemAddr = (uint32_t) buffer;
	config_dma_adc.TransferType = GPDMA_TRANSFERTYPE_P2M;
	config_dma_adc.SrcConn = GPDMA_CONN_ADC;
	config_dma_adc.DMALLI = (uint32_t) &adcLLI;

	GPDMA_Setup(&config_dma_adc);
	GPDMA_ChannelCmd(1, DISABLE);
	GPDMA_ChannelCmd(0, ENABLE);
}

void config_dma_dac(void) {

		dacLLI.SrcAddr = buffer;
		dacLLI.DstAddr = LPC_DAC->DACR;
		dacLLI.NextLLI = &dacLLI;
		dacLLI.Control =
				1024 |
				(1 << 12) |
				(1 << 15) |
				(1 << 18) |
				(1 << 21);

	GPDMA_Channel_CFG_Type config_dma_dac;
	config_dma_dac.ChannelNum = 1;
	config_dma_dac.TransferSize = 1024;
	config_dma_dac.SrcMemAddr = buffer;
	config_dma_dac.DstMemAddr = LPC_DAC->DACR;
	config_dma_dac.TransferType = GPDMA_TRANSFERTYPE_M2P;
	config_dma_dac.DstConn = GPDMA_CONN_DAC;
	config_dma_dac.DMALLI = (uint32_t) &dacLLI;
	GPDMA_Setup(&config_dma_dac);
	GPDMA_ChannelCmd(0, DISABLE);
	GPDMA_ChannelCmd(1, ENABLE);
}

void iniciar_dma(void) {
	GPDMA_Init();
}

void TIM0_IRQHandler(void) {
	static uint8_t n = 1;
	if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
		if (n%2) {
			config_dma_dac();
		} else {
			config_dma_adc();
		}
		n++;
		TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	}
}
