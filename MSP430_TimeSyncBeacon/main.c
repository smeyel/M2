

#include <msp430.h>
#include <ti/mcu/msp430/csl/CSL.h>

#include "tlc5916.h"
#include "gray.h"

#define LED1_ON()		P1OUT |= BIT0
#define LED1_OFF()		P1OUT &= ~BIT0

#define LED2_ON()		P1OUT |= BIT6
#define LED2_OFF()		P1OUT &= ~BIT6

static int run = 0;

int main(int argc, char *argv[]){

	CSL_init();	// Activate Grace-generated configuration

	tlc5916_init();

	#if 1
	tlc5916_write_led(0, 1);
	tlc5916_write_led(2, 1);
	tlc5916_write_led(4, 1);
	tlc5916_send();
	tlc5916_latch();
	#endif
	
	while(1){
		run = 1;
	}

}

static void calc(uint32_t time_ms){

	const int threshold = 32;
	uint32_t low = time_ms % threshold;
	uint32_t high = time_ms / threshold;

	uint32_t run = low;
	uint32_t gray = binaryToGray(high);

	tlc5916_write_led(0 + run, 1);
	tlc5916_write_leds(&gray, 32, 32);

	tlc5916_send();

}

void TIMER_interrupt(void){

	static uint32_t cnt = 0;

	if(run){

		cnt++;

		if(cnt == 500){
			LED1_ON();
			LED2_OFF();
		}
		else if(cnt == 1000){
			cnt = 0;
			LED1_OFF();
			LED2_ON();
		}

	}

}
