

#include <msp430.h>
#include <ti/mcu/msp430/csl/CSL.h>

#include "tlc5916.h"
#include "gray.h"

#define LED1_ON()		P1OUT |= BIT0
#define LED1_OFF()		P1OUT &= ~BIT0

#if !TLC5916_USE_SPI
#define LED2_ON()		P1OUT |= BIT6
#define LED2_OFF()		P1OUT &= ~BIT6
#endif

static int initReady = 0;

int main(int argc, char *argv[]){

	CSL_init();	// Activate Grace-generated configuration

	tlc5916_init();

	#if 0
	tlc5916_write_led(0, 1);
	tlc5916_write_led(2, 1);
	tlc5916_write_led(4, 1);
	tlc5916_send();
	tlc5916_latch();
	#endif
	

	initReady = 1;
	while(1){
	}

}

static void calc(uint32_t time){

	const int threshold = 32;
	uint32_t low = time % threshold;
	uint32_t high = time / threshold;

	uint32_t run = low;
	uint32_t gray = binaryToGray(high);

	tlc5916_write_led(0 + run, 1);
	tlc5916_write_leds(&gray, 32, 32);

	tlc5916_send();

}

void TIMER_interrupt(void){

	static uint32_t time = 0;

	if(initReady){

		tlc5916_latch();

		time++;

		calc(time);

		if((time % 1000) == 0){
			LED1_ON();
		}
		else if((time % 500) == 0){
			LED1_OFF();
		}

	}

}
