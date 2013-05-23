

#include <msp430.h>
#include <ti/mcu/msp430/csl/CSL.h>

#include "config.h"
#include "tlc5916.h"
#include "gray.h"

#define LED1_ON()		P1OUT |= BIT0
#define LED1_OFF()		P1OUT &= ~BIT0

#if(TEST_ON_LAUNCHPAD && !TLC5916_USE_SPI)
#define LED2_ON()		P1OUT |= BIT6
#define LED2_OFF()		P1OUT &= ~BIT6
#endif	//(TEST_ON_LAUNCHPAD && !TLC5916_USE_SPI)

static int initReady;

int main(int argc, char *argv[]){

	initReady = 0;

	CSL_init();	// Activate Grace-generated configuration

	tlc5916_init();
	

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

	tlc5916_write_led((run>0)?(run-1):(threshold-1), 0);
	tlc5916_write_led(0 + run, 1);
	tlc5916_write_leds(&gray, 32, 32);

}

static void run_pattern(uint32_t time){

	uint32_t zero = 0;
	uint32_t pattern = 0x1f;
	const int threshold = 64;
	const int period = 200;
	uint32_t run;

	if(time%period == 0){

		run = time / period % threshold;

		tlc5916_write_leds(&zero, 0, 32);
		tlc5916_write_leds(&zero, 32, 32);
		tlc5916_write_leds(&pattern, run, 32);

	}

}

static int boot_up(uint32_t time){

	static uint16_t pattern = 0;
	const int count = 16;
	const int period = 30;

	if(time%period == 0){

		pattern <<= 1;
		pattern |= 0x0001;

		tlc5916_write_leds(&pattern, 0, 16);
		tlc5916_write_leds(&pattern, 16, 16);
		tlc5916_write_leds(&pattern, 32, 16);
		tlc5916_write_leds(&pattern, 48, 16);

		if(time == period*(count-1))
			return 1;

	}

	return 0;

}

static int boot_down(uint32_t time){

	static uint16_t pattern = 0xffff;
	const int count = 16;
	const int period = 30;

	if(time%period == 0){

		pattern >>= 1;

		tlc5916_write_leds(&pattern, 16, 16);
		tlc5916_write_leds(&pattern, 32, 16);

		//pattern |= 0x8001;
		tlc5916_write_leds(&pattern, 0, 16);
		tlc5916_write_leds(&pattern, 48, 16);
		//pattern &= 0x7fff;

		if(time == period*(count-1))
			return 1;

	}

	return 0;

}

void TIMER_interrupt(void){

	static uint32_t time = 0;
	static int state = 0;

	LED1_ON();

	if(initReady){

		tlc5916_latch();

		switch(state){
			case 0:
				if(boot_up(time)){
					state++;
					time = 0;
				}
				break;
			case 1:
				if(boot_down(time)){
					state++;
					time = 0;
				}
				break;
			case 2:
			default:
				calc(time);
				break;
			case 3:
				run_pattern(time);
				break;
		}

		tlc5916_send();

		time++;

	}

	LED1_OFF();

}
