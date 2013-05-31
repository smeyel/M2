
#include "config.h"
#include "tlc5916.h"
#include <msp430.h>

#include <stdint.h>


#define LED_PER_DRIVER				8
#define DRIVER_COUNT				8
#define LED_COUNT					(LED_PER_DRIVER * DRIVER_COUNT)


//GPIO set/clear commands
#if !TEST_ON_LAUNCHPAD
#define SET_nOE()		P1OUT |= BIT3
#define CLR_nOE()		P1OUT &= ~BIT3

#define SET_LE()		P1OUT |= BIT4
#define CLR_LE()		P1OUT &= ~BIT4

#if !TLC5916_USE_SPI
#define SET_SCK()		P1OUT |= BIT5
#define CLR_SCK()		P1OUT &= ~BIT5

#define SET_SDI()		P1OUT |= BIT6
#define CLR_SDI()		P1OUT &= ~BIT6
#endif	//!TLC5916_USE_SPI

#else	//TEST_ON_LAUNCHPAD

#define SET_nOE()		P2OUT |= BIT0
#define CLR_nOE()		P2OUT &= ~BIT0

#define SET_LE()		P2OUT |= BIT1
#define CLR_LE()		P2OUT &= ~BIT1

#if !TLC5916_USE_SPI
#define SET_SCK()		P2OUT |= BIT2
#define CLR_SCK()		P2OUT &= ~BIT2

#define SET_SDI()		P2OUT |= BIT3
#define CLR_SDI()		P2OUT &= ~BIT3
#endif	//!TLC5916_USE_SPI

#endif	//TEST_ON_LAUNCHPAD


//LED data, this will be sent bit by bit to the drivers
static uint8_t data[DRIVER_COUNT];


//bit masks, we don't need to shift for masking, only get the mask from the array
//the bit on position b is 1 (b=0..7), mask
#define BIT(b)		(0x01 << (b))
static const uint8_t bit_table[] = {
	BIT(0),
	BIT(1),
	BIT(2),
	BIT(3),
	BIT(4),
	BIT(5),
	BIT(6),
	BIT(7)
};


//swap type, because the LEDs are connected to the drivers unsettled
typedef struct{
	uint8_t driver;	//driver index
	uint8_t bit;	//bit mask
}swap_t;

//which LED belongs to which bit from which driver
#define DRIVER(d)			(d)
#define BIT_FROM_PIN(p)			BIT((p)-5)
static const swap_t swap[] = {
	/* LED0  */	{DRIVER(3), BIT_FROM_PIN(11)},
	/* LED1  */	{DRIVER(3), BIT_FROM_PIN(10)},
	/* LED2  */	{DRIVER(3), BIT_FROM_PIN(7)},
	/* LED3  */	{DRIVER(3), BIT_FROM_PIN(6)},
	/* LED4  */	{DRIVER(2), BIT_FROM_PIN(11)},
	/* LED5  */	{DRIVER(2), BIT_FROM_PIN(10)},
	/* LED6  */	{DRIVER(2), BIT_FROM_PIN(7)},
	/* LED7  */	{DRIVER(2), BIT_FROM_PIN(6)},
	/* LED8  */	{DRIVER(1), BIT_FROM_PIN(11)},
	/* LED9  */	{DRIVER(1), BIT_FROM_PIN(10)},
	/* LED10 */	{DRIVER(1), BIT_FROM_PIN(7)},
	/* LED11 */	{DRIVER(1), BIT_FROM_PIN(6)},
	/* LED12 */	{DRIVER(0), BIT_FROM_PIN(11)},
	/* LED13 */	{DRIVER(0), BIT_FROM_PIN(10)},
	/* LED14 */	{DRIVER(0), BIT_FROM_PIN(7)},
	/* LED15 */	{DRIVER(0), BIT_FROM_PIN(6)},
	/* LED16 */	{DRIVER(3), BIT_FROM_PIN(12)},
	/* LED17 */	{DRIVER(3), BIT_FROM_PIN(9)},
	/* LED18 */	{DRIVER(3), BIT_FROM_PIN(8)},
	/* LED19 */	{DRIVER(3), BIT_FROM_PIN(5)},
	/* LED20 */	{DRIVER(2), BIT_FROM_PIN(12)},
	/* LED21 */	{DRIVER(2), BIT_FROM_PIN(9)},
	/* LED22 */	{DRIVER(2), BIT_FROM_PIN(8)},
	/* LED23 */	{DRIVER(2), BIT_FROM_PIN(5)},
	/* LED24 */	{DRIVER(1), BIT_FROM_PIN(12)},
	/* LED25 */	{DRIVER(1), BIT_FROM_PIN(9)},
	/* LED26 */	{DRIVER(1), BIT_FROM_PIN(8)},
	/* LED27 */	{DRIVER(1), BIT_FROM_PIN(5)},
	/* LED28 */	{DRIVER(0), BIT_FROM_PIN(12)},
	/* LED29 */	{DRIVER(0), BIT_FROM_PIN(9)},
	/* LED30 */	{DRIVER(0), BIT_FROM_PIN(8)},
	/* LED31 */	{DRIVER(0), BIT_FROM_PIN(5)},
	/* LED32 */	{DRIVER(4), BIT_FROM_PIN(5)},
	/* LED33 */	{DRIVER(4), BIT_FROM_PIN(8)},
	/* LED34 */	{DRIVER(4), BIT_FROM_PIN(9)},
	/* LED35 */	{DRIVER(4), BIT_FROM_PIN(12)},
	/* LED36 */	{DRIVER(5), BIT_FROM_PIN(5)},
	/* LED37 */	{DRIVER(5), BIT_FROM_PIN(8)},
	/* LED38 */	{DRIVER(5), BIT_FROM_PIN(9)},
	/* LED39 */	{DRIVER(5), BIT_FROM_PIN(12)},
	/* LED40 */	{DRIVER(6), BIT_FROM_PIN(5)},
	/* LED41 */	{DRIVER(6), BIT_FROM_PIN(8)},
	/* LED42 */	{DRIVER(6), BIT_FROM_PIN(9)},
	/* LED43 */	{DRIVER(6), BIT_FROM_PIN(12)},
	/* LED44 */	{DRIVER(7), BIT_FROM_PIN(5)},
	/* LED45 */	{DRIVER(7), BIT_FROM_PIN(8)},
	/* LED46 */	{DRIVER(7), BIT_FROM_PIN(9)},
	/* LED47 */	{DRIVER(7), BIT_FROM_PIN(12)},
	/* LED48 */	{DRIVER(4), BIT_FROM_PIN(6)},
	/* LED49 */	{DRIVER(4), BIT_FROM_PIN(7)},
	/* LED50 */	{DRIVER(4), BIT_FROM_PIN(10)},
	/* LED51 */	{DRIVER(4), BIT_FROM_PIN(11)},
	/* LED52 */	{DRIVER(5), BIT_FROM_PIN(6)},
	/* LED53 */	{DRIVER(5), BIT_FROM_PIN(7)},
	/* LED54 */	{DRIVER(5), BIT_FROM_PIN(10)},
	/* LED55 */	{DRIVER(5), BIT_FROM_PIN(11)},
	/* LED56 */	{DRIVER(6), BIT_FROM_PIN(6)},
	/* LED57 */	{DRIVER(6), BIT_FROM_PIN(7)},
	/* LED58 */	{DRIVER(6), BIT_FROM_PIN(10)},
	/* LED59 */	{DRIVER(6), BIT_FROM_PIN(11)},
	/* LED60 */	{DRIVER(7), BIT_FROM_PIN(6)},
	/* LED61 */	{DRIVER(7), BIT_FROM_PIN(7)},
	/* LED62 */	{DRIVER(7), BIT_FROM_PIN(10)},
	/* LED63 */	{DRIVER(7), BIT_FROM_PIN(11)},
};

//GPIO pin init
static void gpio_init(void){

	//Grace initializates the pins
	//here are only init states assigned,
	//if Grace wouldn't do that
	SET_nOE();
	CLR_LE();
#if !TLC5916_USE_SPI
	CLR_SCK();
	CLR_SDI();
#endif	//!TLC5916_USE_SPI
	
}



#if !TLC5916_USE_SPI

//send one bit to the SDI pin of the driver
//b=0: low
//b!=0: high
static void send_one_bit(int b){
	
	if(b)	SET_SDI();
	else	CLR_SDI();
	
	SET_SCK();
	CLR_SCK();

}

//send all the bits of the data array to the drivers
static void send(void){

	int i;
	int j;

	for(i=DRIVER_COUNT ; i-- > 0 ; )
		for(j=LED_PER_DRIVER ; j-- > 0 ; )
			//no shift is required for generating bit mask, only get it from the array
			send_one_bit(data[i] & bit_table[j]);
			//send_one_bit(data[i] & BIT(j));

}

#else	//TLC5916_USE_SPI

//send one byte to the SDI pin of the driver via SPI
//MSB first
static void send_one_byte(uint8_t byte){

	USISRL = byte;				// Load shift register with data byte to be TXed
	USICNT = 8;					// Load bit-counter to send/receive data byte
	while(!(USIIFG & USICTL1));	// Loop until data byte transmitted
	byte = USISRL;				// Read out the received data

}

//send all the bits of the data array to the drivers
static void send(void){

	int i;

	for(i=DRIVER_COUNT ; i-- > 0 ; )
		send_one_byte(data[i]);

}

#endif	//TLC5916_USE_SPI

//generate the latch impulse
static void latch(void){
	
	SET_LE();
	CLR_LE();

}

//the appropriate bit of the appropriate element in the data[] array is setted/cleared,
//according to the required state of the LED
//the swap[] table is used
//on_off!=0: on => 0
//on_off=0: off => 1
static void write_led(int led, int on_off){

	uint8_t driver;
	uint8_t bit;

	if((0 <= led) && (led < LED_COUNT)){

		driver = swap[led].driver;
		bit = swap[led].bit;

		if(on_off)	data[driver] |= bit;
		else		data[driver] &= ~bit;

	}

}

//more LED states are setted/cleared,
//according to the bits in the bytes pointed by the pointer
static void write_leds(const void* states, int from_led, int led_count){

	const int size = 8;
	const uint8_t* st = (const uint8_t*)states;
	
	int b;

	if(from_led >= LED_COUNT)
		return;

	if(from_led + led_count > LED_COUNT)
		led_count = LED_COUNT - from_led;

	//led: from_led + b
	//on_off: the bit on position (b%size) in the byte on position (b/size) in the bytes pointed by st
	for(b=0 ; b < led_count ; b++)
		//write_led(from_led + b, (*(st+(b/size))) & bit_table[b%size]);
		//size=8, so the division and remainder generation can be raplaced by shifting and masking (the compiler does not realize)
		write_led(from_led + b, (*(st+(b>>3))) & bit_table[b&0x07]);

}

//initialization of the variables, GPIO pins, switching off all the LEDs, enabling the outputs
static void init(void){

	int i;
	
	gpio_init();

	for(i=DRIVER_COUNT ; i-- > 0 ; )
		data[i] = 0;
	send();

	latch();
	CLR_nOE();

}

void tlc5916_send(void){
	send();
}
void tlc5916_latch(void){
	latch();
}
void tlc5916_write_led(int led, int on_off){
	write_led(led, on_off);
}
void tlc5916_write_leds(const void* states, int from_led, int led_count){
	write_leds(states, from_led, led_count);
}
void tlc5916_init(void){
	init();
}
