/*
 * Enciende un led conectado al pinP0.22 cuando el tiempo entre dos
 * eventos consecutivos asociados a un flanco ascendente en el pin P1.26 debido a la
 * interrupción del TMR0, sea mayor o igual a 1s
 */
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"

#define PORT_ZERO ((uint8_t) 0)
#define LED22 ((uint32_t) (1 << 22))
#define OUTPUT ((uint8_t) 1)
#define INPUT ((uint8_t) 0)

volatile uint32_t currentCapValue = 0;
volatile uint32_t lastCapValue = 0;
volatile uint8_t capFlag = 0;

void configPins(void);
void configTimer(void);
void encenderLed(void);

int main(void) {
	configPins();
	configTimer();
	while(1) {

	}
	return 0;
}

void configPins(void) {
	//les P0.22
	PINSEL_CFG_Type config_led;
	config_led.Portnum = PINSEL_PORT_0;
	config_led.Pinnum = PINSEL_PIN_22;
	config_led.Funcnum = PINSEL_FUNC_0;
	config_led.Pinmode = PINSEL_PINMODE_TRISTATE;
	config_led.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_led);

	GPIO_SetDir(PORT_ZERO, LED22, OUTPUT);

	//pulsador P1.26
	PINSEL_CFG_Type config_pulsador;
	config_pulsador.Portnum = PINSEL_PORT_1;
	config_pulsador.Pinnum = PINSEL_PIN_26;
	config_pulsador.Funcnum = PINSEL_FUNC_3;
	config_pulsador.Pinmode = PINSEL_PINMODE_PULLDOWN;
	config_pulsador.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&config_pulsador);
}

void configTimer() {
	TIM_COUNTERCFG_Type config_timer;
	config_timer.CounterOption = TIM_COUNTER_INCAP0;

	TIM_CAPTURECFG_Type config_cap;
	config_cap.CaptureChannel = 0;
	config_cap.RisingEdge = ENABLE;
	config_cap.FallingEdge = DISABLE;
	config_cap.IntOnCaption = ENABLE;

	TIM_Init(LPC_TIM0, TIM_COUNTER_RISING_MODE, &config_timer);
	TIM_ConfigCapture(LPC_TIM0, &config_cap);
	TIM_Cmd(LPC_TIM0, ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void) {
	if(TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT)) {
		currentCapValue = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
		if (capFlag == 0) {
			//almacena capture
			lastCapValue = currentCapValue;
		} else {
			uint32_t diffTicks = (currentCapValue - lastCapValue);
			float tickTime = 1.0f / 25000000.0f;
			float diffTime = diffTicks * tickTime;
			if (diffTime > 1.0f) {
				encenderLed();
			} else {
				lastCapValue = currentCapValue;
			}
		}
	}
	TIM_ClearIntPending(LPC_TIM0, TIM_CR0_INT);
}

void encenderLed(void) {
	FIO_SetValue(PORT_ZERO, LED22);
}
