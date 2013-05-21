
#include "time.h"
#include "tlc5916.h"
#include "gray.h"


static uint32_t time_ms = 0;
static const int interrupt_per_ms = 1;


void time_init(void){
}

static void calc(void){

	const int threshold = 32;
	uint32_t low = time_ms % threshold;
	uint32_t high = time_ms / threshold;

	uint32_t run = low;
	uint32_t gray = binaryToGray(high);
	
	tlc5916_write_led(0 + run, 1);
	tlc5916_write_leds(&gray, 32, 32);
	
	tlc5916_send();
	
}

static void ms(void){

	tlc5916_latch();
	
	time_ms++;
	
	calc();
	
}

void time_interrupt(void){

	static int counter = 0;
	
	counter++;

	if(counter == interrupt_per_ms){
		counter = 0;
		ms();
	}

}
