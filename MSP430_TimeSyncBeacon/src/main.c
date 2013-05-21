

#include <msp430.h>
#include <ti/mcu/msp430/Grace.h>

#include "tlc5916.h"
#include "gray.h"

int main(void){

	Grace_init();	// Activate Grace-generated configuration

	/* Start Timer_A */
	TA0CTL |= MC_1;	/* Start timer in up mode */

	tlc5916_init();

	#if 1
	tlc5916_write_led(0, 1);
	tlc5916_write_led(2, 1);
	tlc5916_write_led(4, 1);
	tlc5916_send();
	tlc5916_latch();
	#endif
	
	while(1){

	}

}

void calc(uint32_t time_ms){

	const int threshold = 32;
	uint32_t low = time_ms % threshold;
	uint32_t high = time_ms / threshold;

	uint32_t run = low;
	uint32_t gray = binaryToGray(high);

	tlc5916_write_led(0 + run, 1);
	tlc5916_write_leds(&gray, 32, 32);

	tlc5916_send();

}
