/*
 * Programa para control de riego automatico controlado por bluetooth con LPC1769
 * Configuracion:
 * - Bomba de agua de 12V controlado por PWM: P2.0
 * - Sensor de humedad de suelo : P0.23
 * - Sensor de distancia HC-SR04 : P0.4(ECHO pin) -  P0.5(TRIG pin)
 * - Modulo Bluetooth HC-05 : P0.10(RX sensor) - P0.11(TX sensor)
 * - LED Rojo/Buzzer : P0.25
 * - LED Verde : P0.24
 */
#include "LPC17xx.h"
#include "../Drivers/inc/lpc17xx_pinsel.h"
#include "../Drivers/inc/lpc17xx_gpio.h"
#include "../Drivers/inc/lpc17xx_uart.h"
#include "../Drivers/inc/lpc17xx_timer.h"
#include "../Drivers/inc/lpc17xx_gpdma.h"
#include "../Drivers/inc/lpc17xx_adc.h"
#include "../Drivers/inc/lpc17xx_pwm.h"

//===============================================================================================================================
//====================================================== MACROS Y VARIABLES =====================================================
//===============================================================================================================================

//Macros
#define OUTPUT 	(uint8_t)1
#define INTPUT 	(uint8_t)0

#define PORT_0 	(uint8_t)0
#define PORT_2 	(uint8_t)2

#define PUMP 	(uint32_t)(1 << 0)
#define RED_LED (uint32_t)(1 << 25)
#define GREEN_LED (uint32_t)(1 << 24)


#define TRIG (uint32_t)(1 << 5)
#define ECHO (uint32_t)(1 << 4)

#define TX_BUFFER_SIZE 10
#define RX_BUFFER_SIZE 4


#define PWM_PERIOD 25000

//-------------------------------Variables-------------------------------
//ADC
volatile uint16_t adc_sample = 0;
//UART
uint8_t TxBuffer[TX_BUFFER_SIZE] = {0, 0, 0, 0x25, 0x0A, 0, 0, 0, 0x25, 0x0A}; //0x25 es % y 0x0A es salto de linea
uint8_t RxBuffer[RX_BUFFER_SIZE] = {0, 0, 0, 0};
volatile uint8_t receivedData = 0; //dato  recibido
//sensor de distancia
volatile uint32_t distanceTime = 0;
volatile uint32_t distance = 0;
volatile uint32_t startCap = 0;
volatile uint32_t endCap = 0;
uint8_t wtr_lvl = 0;
//control de la bomba
uint8_t autoFlg = 0;
uint8_t pump_on = 0;
uint8_t pause = 0;
uint8_t duty = 55; //duty cicle inicial (PWM)
//sensor de humedad de suelo
const uint16_t MAX_VAL_MOIS = 3474;
const uint16_t MIN_VAL_MOIS = 2052;
const uint16_t RANGE_VAL_MOIS = MAX_VAL_MOIS - MIN_VAL_MOIS;
uint8_t moisPrct = 0; //porcentaje actual
uint8_t percent = 80; //porcentaje maximo
uint8_t minMois = 10;

//===============================================================================================================================
//======================================================= CONFIGURACIONES =======================================================
//===============================================================================================================================

//------------------------------
//Configuracion de los pines
//------------------------------
void cfgPin(void){
	//PWM1.1
	PINSEL_CFG_Type cfgPumpPin = {PINSEL_PORT_2, PINSEL_PIN_0, PINSEL_FUNC_1, PINSEL_PINMODE_PULLDOWN, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgPumpPin);
	//TRIG PIN
	PINSEL_CFG_Type cfgTrigPin = {PINSEL_PORT_0, PINSEL_PIN_5, PINSEL_FUNC_0, PINSEL_PINMODE_PULLDOWN, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgTrigPin);
	GPIO_SetDir(0, 1 << 5, 1);
	GPIO_ClearValue(0, 1 << 5);
	//TIMER2 - CAP2.0/ECHO
	PINSEL_CFG_Type cfgTimerCapPin = {PINSEL_PORT_0, PINSEL_PIN_4, PINSEL_FUNC_3, PINSEL_PINMODE_PULLDOWN, PINSEL_PINMODE_NORMAL}; //P0.4
	PINSEL_ConfigPin(&cfgTimerCapPin);
	//UART
	PINSEL_CFG_Type cfgTXD2Pin 	= {PINSEL_PORT_0, PINSEL_PIN_10, PINSEL_FUNC_1, 0, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgTXD2Pin);
	PINSEL_CFG_Type cfgRXD2Pin 	= {PINSEL_PORT_0, PINSEL_PIN_11, PINSEL_FUNC_1, 0, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgRXD2Pin);
	//TIMER0 - MAT0.0
	PINSEL_CFG_Type cfgMat0Pin_CH0 = {PINSEL_PORT_1, PINSEL_PIN_28, PINSEL_FUNC_3, 0,PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgMat0Pin_CH0);
	//TIMER1 - MAT1.0
	PINSEL_CFG_Type cfgMat1Pin_CH0 = {PINSEL_PORT_1, PINSEL_PIN_22, PINSEL_FUNC_3, 0,PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgMat1Pin_CH0);
	//ADC - AD0.0
	PINSEL_CFG_Type cfgAD0Pin_CH0 = {PINSEL_PORT_0, PINSEL_PIN_23, PINSEL_FUNC_1, PINSEL_PINMODE_TRISTATE, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgAD0Pin_CH0);
	//GREEN LED
	PINSEL_CFG_Type cfgGreenLedPin = {PINSEL_PORT_0, PINSEL_PIN_24, PINSEL_FUNC_0, PINSEL_PINMODE_PULLDOWN, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgGreenLedPin);
	GPIO_SetDir(PORT_0, GREEN_LED, OUTPUT);
	GPIO_ClearValue(PORT_0, GREEN_LED);
	//RED LED
	PINSEL_CFG_Type cfgRedLedPin = {PINSEL_PORT_0, PINSEL_PIN_25, PINSEL_FUNC_0, PINSEL_PINMODE_PULLDOWN, PINSEL_PINMODE_NORMAL};
	PINSEL_ConfigPin(&cfgRedLedPin);
	GPIO_SetDir(PORT_0, RED_LED, OUTPUT);
	GPIO_ClearValue(PORT_0, RED_LED);
}

//------------------------------
//Configuracion de Timers
//------------------------------

void cfgTimer0(void){
	//configuracion modo del timer
	TIM_TIMERCFG_Type cfgTimer0Mode;
	cfgTimer0Mode.PrescaleOption = TIM_PRESCALE_USVAL;
	cfgTimer0Mode.PrescaleValue = 1000;
	//configuracion de MATCH0.0
	TIM_MATCHCFG_Type cfgTimerMatch00;
	cfgTimerMatch00.MatchChannel 		= 0;
	cfgTimerMatch00.MatchValue 			= 999;  ///RECORDA CAMBIAR POR 4999
	cfgTimerMatch00.IntOnMatch 			= ENABLE;
	cfgTimerMatch00.ResetOnMatch 		= ENABLE;
	cfgTimerMatch00.StopOnMatch 		= DISABLE;
	cfgTimerMatch00.ExtMatchOutputType 	= TIM_EXTMATCH_NOTHING;
	//inicializacion del timer
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &cfgTimer0Mode);
	TIM_ConfigMatch(LPC_TIM0, &cfgTimerMatch00);
	TIM_Cmd(LPC_TIM0, DISABLE);
	//configuracion NVIC
	NVIC_SetPriority(TIMER0_IRQn, 1);
}

void cfgTimer1(void){
	//configuracion modo del timer
	TIM_TIMERCFG_Type cfgTimer1Mode;
	cfgTimer1Mode.PrescaleOption = TIM_PRESCALE_USVAL;
	cfgTimer1Mode.PrescaleValue = 1000;
	//configuracion de MATCH0.0
	TIM_MATCHCFG_Type cfgTimerMatch10;
	cfgTimerMatch10.MatchChannel 		= 0;
	cfgTimerMatch10.MatchValue 			= 999;
	cfgTimerMatch10.IntOnMatch 			= DISABLE;
	cfgTimerMatch10.ResetOnMatch 		= ENABLE;
	cfgTimerMatch10.StopOnMatch 		= DISABLE;
	cfgTimerMatch10.ExtMatchOutputType 	= TIM_EXTMATCH_TOGGLE;
	//inicializacion del timer
	TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &cfgTimer1Mode);
	TIM_ConfigMatch(LPC_TIM1, &cfgTimerMatch10);
	TIM_Cmd(LPC_TIM1, ENABLE);
}

void cfgTimer2(void)
{
	TIM_TIMERCFG_Type cfgTimer2Mode;
	cfgTimer2Mode.PrescaleOption = TIM_PRESCALE_USVAL;
	cfgTimer2Mode.PrescaleValue = 1;

	TIM_CAPTURECFG_Type cfgTimerCap2;
	cfgTimerCap2.CaptureChannel = 0;
	cfgTimerCap2.RisingEdge = ENABLE;
	cfgTimerCap2.FallingEdge = ENABLE;
	cfgTimerCap2.IntOnCaption = ENABLE;

	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &cfgTimer2Mode);
	TIM_ConfigCapture(LPC_TIM2, &cfgTimerCap2);
	TIM_Cmd(LPC_TIM2, ENABLE);

	NVIC_SetPriority(TIMER2_IRQn, 2);
	NVIC_EnableIRQ(TIMER2_IRQn);
}

//------------------------------
//Configuracion de PWM
//------------------------------

void cfgPWM(void)
{

	PWM_TIMERCFG_Type cfgPWMTimer;
	PWM_MATCHCFG_Type cfgPWM_MR0; // Mat 0 - PERIODO
	PWM_MATCHCFG_Type cfgPWM_MR1; // Mat 1 - DUTY

	cfgPWMTimer.PrescaleOption = PWM_TIMER_PRESCALE_TICKVAL;
	cfgPWMTimer.PrescaleValue = 1;
	PWM_Init(LPC_PWM1, PWM_MODE_TIMER, &cfgPWMTimer);

	//Configuración de MR0 (Periodo)
	cfgPWM_MR0.MatchChannel = 0;
	cfgPWM_MR0.IntOnMatch = DISABLE;
	cfgPWM_MR0.StopOnMatch = DISABLE;
	cfgPWM_MR0.ResetOnMatch = ENABLE; // MR0 REINICIA el contador
	PWM_ConfigMatch(LPC_PWM1, &cfgPWM_MR0);
	PWM_MatchUpdate(LPC_PWM1, 0, PWM_PERIOD, PWM_MATCH_UPDATE_NOW);

	//Configuración de MR1 (Duty)
	cfgPWM_MR1.MatchChannel = 1;
	cfgPWM_MR1.IntOnMatch = DISABLE;
	cfgPWM_MR1.StopOnMatch = DISABLE;
	cfgPWM_MR1.ResetOnMatch = DISABLE; // MR1 NO debe reiniciar el contador
	PWM_ConfigMatch(LPC_PWM1, &cfgPWM_MR1);
	PWM_MatchUpdate(LPC_PWM1, 1, 0, PWM_MATCH_UPDATE_NOW);

	PWM_CounterCmd(LPC_PWM1, ENABLE);
	PWM_ChannelCmd(LPC_PWM1, 1, ENABLE);
	PWM_Cmd(LPC_PWM1, ENABLE);
}

//------------------------------
//Configuracion del UART
//------------------------------

void cfgUart(void)
{
	//configuracion inicial de UART
	UART_CFG_Type cfgUART2;
	UART_ConfigStructInit(&cfgUART2); //Inicializa con 9600 de baudrate, 8 bits de datos, sin bit de paridad
	UART_Init(LPC_UART2, &cfgUART2);
	//configuracion de FIFO
	UART_FIFO_CFG_Type cfgUART2FIFO;
	UART_FIFOConfigStructInit(&cfgUART2FIFO);
	cfgUART2FIFO.FIFO_DMAMode = ENABLE;
	UART_FIFOConfig(LPC_UART2, &cfgUART2FIFO);
	//configuracion de interrupcion por UART
	UART_IntConfig(LPC_UART2, UART_INTCFG_RBR, ENABLE); //interrupcion por recepcion
	UART_TxCmd(LPC_UART2, ENABLE);
	//configuracion NVIC
	NVIC_SetPriority(UART2_IRQn, 4);
	NVIC_EnableIRQ(UART2_IRQn);
}

//------------------------------
//Configuracion del ADC
//------------------------------

void cfgADC(void)
{
	ADC_Init(LPC_ADC,100000);
	ADC_BurstCmd(LPC_ADC, DISABLE);
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT10);
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);

	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, SET);

	NVIC_SetPriority(ADC_IRQn, 5);
	NVIC_EnableIRQ(ADC_IRQn);
}

//------------------------------
//Configuracion del DMA
//------------------------------

//M2P
void cfgDmaUart(void)
{
	NVIC_DisableIRQ(DMA_IRQn);
	GPDMA_Init();

	GPDMA_Channel_CFG_Type cfgDmaUART;
	cfgDmaUART.ChannelNum = 1;
	cfgDmaUART.TransferSize = TX_BUFFER_SIZE;
	cfgDmaUART.TransferWidth = 0;
	cfgDmaUART.SrcMemAddr = (uint32_t)TxBuffer;
	cfgDmaUART.DstMemAddr = 0;
	cfgDmaUART.TransferType = GPDMA_TRANSFERTYPE_M2P;
	cfgDmaUART.SrcConn = 0;
	cfgDmaUART.DstConn = GPDMA_CONN_UART2_Tx;
	cfgDmaUART.DMALLI = 0;

	GPDMA_Setup(&cfgDmaUART);
}

//===============================================================================================================================
//=========================================================== FUNCIONES =========================================================
//===============================================================================================================================

void setDuty(uint8_t duty_percent)
{
    if (duty_percent > 100) duty_percent = 100;

    uint32_t new_match_value = (PWM_PERIOD * duty_percent) / 100;
    PWM_MatchUpdate(LPC_PWM1, 1, new_match_value, PWM_MATCH_UPDATE_NOW);
}

uint8_t getHysteresis()
{
	return (uint8_t)(5 + ((uint32_t)(percent - 10) * 10) / 70);
}

void pumpOn(void)
{
	pump_on  = 1;
	setDuty(duty);
}

void pumpOff(void)
{
	pump_on  = 0;
	setDuty(0);
}

void alarmOn(void)
{
	GPIO_ClearValue(PORT_0, GREEN_LED);

	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void alarmOff(void)
{
	TIM_ResetCounter(LPC_TIM0);
	TIM_Cmd(LPC_TIM0, DISABLE);
	NVIC_DisableIRQ(TIMER0_IRQn);

	GPIO_ClearValue(PORT_0, RED_LED);
	GPIO_SetValue(PORT_0, GREEN_LED);
}

void checkMoisture(void)
{
    minMois = (percent - getHysteresis());

    if (pump_on)
    {
        if (moisPrct >= percent)pumpOff(); //detener cuando alcanzamos o superamos la humedad deseada
    }
    else
    {
        if (moisPrct <= minMois)pumpOn(); //arrancar solo cuando la humedad cae por debajo del límite inferior
    }
}

void systemUpdate(void)
{
	if(pause){
		pumpOff();
		return;
	}

	if(autoFlg)checkMoisture();
	else if (moisPrct >= percent)pumpOff();
}

void selectMode(uint8_t modeSel)
{
	switch(modeSel)
	{
		case 'a'://modo automatico
			autoFlg = 1;
			break;

		case 'm'://modo manual
			if(!autoFlg)break;
			pumpOff();
			autoFlg = 0;
			break;
	}
}

void manualControl(uint8_t pmCtrl)
{
	switch(pmCtrl)
	{
		case 'd'://off
			pumpOff();
			break;

		case 'e'://on
			checkMoisture();
			break;

	}
}

void lowWaterLevel(void)
{
	if(wtr_lvl < 10)
	{//pause
		pumpOff();
		pause = 1;
		//alarma
		alarmOn();
	}
	else
	{//resume
		alarmOff();
		pause = 0;
	}

}

uint8_t convertNumHex(uint8_t conv)
{
	return 0x30 + conv;
}

uint8_t convertHexNum(uint8_t conv)
{
	return conv - 0x30;
}

/*
 * maximo y minimo deseados (10% - 80%)
 * 3332 -> 10%
 * 2336 -> 80%
 */
void setMoisturePercent(void)
{
	if(adc_sample >=  MAX_VAL_MOIS) moisPrct = 0;
	else if(adc_sample <= MIN_VAL_MOIS) moisPrct = 100;
	else moisPrct = ((uint32_t)(MAX_VAL_MOIS - adc_sample) * 100) / RANGE_VAL_MOIS;

	TxBuffer[0] = convertNumHex(moisPrct / 100);       // centena
	TxBuffer[1] = convertNumHex((moisPrct / 10) % 10); // decena
	TxBuffer[2] = convertNumHex(moisPrct % 10);        // unidad
}

void setWaterLevel(void)
{
	if(distance >=  30) wtr_lvl = 0;
	else if(distance <= 5) wtr_lvl = 100;
	else wtr_lvl = ((uint32_t)(30 - distance) * 100) / (30 -  5);

	TxBuffer[5] = convertNumHex(wtr_lvl / 100);       // centena
	TxBuffer[6] = convertNumHex((wtr_lvl / 10) % 10); // decena
	TxBuffer[7] = convertNumHex(wtr_lvl % 10);        // unidad
}

void setPotency(void)
{
	uint8_t pot = convertHexNum(RxBuffer[1]) * 100 +
			      convertHexNum(RxBuffer[2]) * 10  +
				  convertHexNum(RxBuffer[3]);

	if(pot == 0)duty = 0;
	else duty = 35 + (65 * pot) / 100;

	if(pump_on)setDuty(duty);
}

void setMaxMoisture(void)
{
	percent = convertHexNum(RxBuffer[1]) * 100 +
			  convertHexNum(RxBuffer[2]) * 10  +
			  convertHexNum(RxBuffer[3]);

	if(percent > 80)percent = 80;
	if(percent < 10)percent = 0;
}
//===============================================================================================================================
//=========================================================== HANDLERS ==========================================================
//===============================================================================================================================

void UART2_IRQHandler(void)
{
	if(UART_GetLineStatus(LPC_UART2) & UART_LSR_RDR){
			if(pause)return;

			receivedData = UART_ReceiveByte(LPC_UART2);

		    static uint8_t rxIdx = 0;
		    uint8_t finishRx = 0;

		    //Fin por ENTER
		    if(receivedData == '\n' || receivedData == '\r')finishRx = 1;
		    else
		    {
		        //Limite de 4 Bytes max
		        if (rxIdx < 4)RxBuffer[rxIdx++] = receivedData;
		        if (rxIdx == 4)finishRx = 1;
		    }

		    if (finishRx)
		    {
		        if (rxIdx >= 1 && rxIdx <= 4)
		        {
		        	if(RxBuffer[0] == 'h')setMaxMoisture();
		        	if(RxBuffer[0] == 'p')setPotency();

		        	systemUpdate();
		        	selectMode(RxBuffer[0]);
					if(!autoFlg)manualControl(RxBuffer[0]);
		        }
		        rxIdx = 0;
		    }
		}
}

void ADC_IRQHandler(void)
{
	while(!(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE))){}

	adc_sample = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0) & 0x0FFF;

	setMoisturePercent();
	systemUpdate();

	GPDMA_ChannelCmd(1, DISABLE);
	cfgDmaUart();
	GPDMA_ChannelCmd(1, ENABLE);
}

void TIMER0_IRQHandler(void)
{
	static uint8_t togg = 1;

	if(togg)GPIO_SetValue(PORT_0, RED_LED);
	else GPIO_ClearValue(PORT_0, RED_LED);

	togg = !togg;

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void TIMER2_IRQHandler(void)
{
	uint32_t cap = TIM_GetCaptureValue(LPC_TIM2, 0);

	if(GPIO_ReadValue(PORT_0) & ECHO)
	{
		startCap = cap;
	}
	else
	{
		endCap = cap;

		if(endCap >= startCap)distanceTime = endCap - startCap;
		else distanceTime = (0xFFFFFFFF - startCap) + endCap + 1; //si se pasa

		distance = (distanceTime * 343) / 20000;

		setWaterLevel();
		lowWaterLevel();
		systemUpdate();
	}

	TIM_ClearIntCapturePending(LPC_TIM2, TIM_CR0_INT);
}

void SysTick_Handler(void)//cada 10 us y periodo de 100ms
{
	static uint32_t vTiks = 0;

	if(vTiks < 1) GPIO_SetValue(PORT_0, TRIG);
	else GPIO_ClearValue(PORT_0, TRIG);

	vTiks = (vTiks + 1) % 10000;

	SysTick -> CTRL &= SysTick -> CTRL;
}

//===============================================================================================================================
//====================================================== PROGRAMA PRINCIPAL =====================================================
//===============================================================================================================================


int main()
{
	cfgPin();
	cfgADC();
	cfgUart();

	cfgTimer0();
	cfgTimer1();
	cfgTimer2();

	SysTick_Config(999);
	NVIC_SetPriority(SysTick_IRQn, 3);

	cfgPWM();
	pumpOff();

	cfgDmaUart();

	while(1){}
	return 0;
}
