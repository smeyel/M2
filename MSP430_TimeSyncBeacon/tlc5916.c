
#include "tlc5916.h"
#include <msp430.h>

#include <stdint.h>


#define LED_PER_DRIVER				8
#define DRIVER_COUNT				8
#define LED_COUNT					(LED_PER_DRIVER * DRIVER_COUNT)


//GPIO kezelés
//TODO: megírni
#define SET_nOE()		P2OUT |= BIT0
#define CLR_nOE()		P2OUT &= ~BIT0

#define SET_LE()		P2OUT |= BIT1
#define CLR_LE()		P2OUT &= ~BIT1

#if !TLC5916_USE_SPI
#define SET_SCK()		P2OUT |= BIT2
#define CLR_SCK()		P2OUT &= ~BIT2

#define SET_SDI()		P2OUT |= BIT3
#define CLR_SDI()		P2OUT &= ~BIT3
#endif	//TLC5916_USE_SPI


//LED adatok, ebbe rakjuk össze amit ki fogunk küldeni
static uint8_t data[DRIVER_COUNT];


//b-edik bit 1-es (b=0..7), maszk
//d-edik driver indexe
#define BIT(b)		(0x01 << (b))
#define DRIVER(d)	(d)

//bit maszkok, hogy ne kelljen shiftelgetni, csak innen kiindexelni
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


//swap típus, mert a LED-ek összevissza lesznek bekötve
typedef struct{
	uint8_t driver;
	uint8_t bit;
}swap_t;

//melyik LED melyik driver hányas bit-jénél van
static const swap_t swap[] = {
	/* LED0  */	{DRIVER(0), BIT(0)},
	/* LED1  */	{DRIVER(0), BIT(1)},
	/* LED2  */	{DRIVER(0), BIT(2)},
	/* LED3  */	{DRIVER(0), BIT(3)},
	/* LED4  */	{DRIVER(0), BIT(4)},
	/* LED5  */	{DRIVER(0), BIT(5)},
	/* LED6  */	{DRIVER(0), BIT(6)},
	/* LED7  */	{DRIVER(0), BIT(7)},
	/* LED8  */	{DRIVER(1), BIT(0)},
	/* LED9  */	{DRIVER(1), BIT(1)},
	/* LED10 */	{DRIVER(1), BIT(2)},
	/* LED11 */	{DRIVER(1), BIT(3)},
	/* LED12 */	{DRIVER(1), BIT(4)},
	/* LED13 */	{DRIVER(1), BIT(5)},
	/* LED14 */	{DRIVER(1), BIT(6)},
	/* LED15 */	{DRIVER(1), BIT(7)},
	/* LED16 */	{DRIVER(2), BIT(0)},
	/* LED17 */	{DRIVER(2), BIT(1)},
	/* LED18 */	{DRIVER(2), BIT(2)},
	/* LED19 */	{DRIVER(2), BIT(3)},
	/* LED20 */	{DRIVER(2), BIT(4)},
	/* LED21 */	{DRIVER(2), BIT(5)},
	/* LED22 */	{DRIVER(2), BIT(6)},
	/* LED23 */	{DRIVER(2), BIT(7)},
	/* LED24 */	{DRIVER(3), BIT(0)},
	/* LED25 */	{DRIVER(3), BIT(1)},
	/* LED26 */	{DRIVER(3), BIT(2)},
	/* LED27 */	{DRIVER(3), BIT(3)},
	/* LED28 */	{DRIVER(3), BIT(4)},
	/* LED29 */	{DRIVER(3), BIT(5)},
	/* LED30 */	{DRIVER(3), BIT(6)},
	/* LED31 */	{DRIVER(3), BIT(7)},
	/* LED32 */	{DRIVER(4), BIT(0)},
	/* LED33 */	{DRIVER(4), BIT(1)},
	/* LED34 */	{DRIVER(4), BIT(2)},
	/* LED35 */	{DRIVER(4), BIT(3)},
	/* LED36 */	{DRIVER(4), BIT(4)},
	/* LED37 */	{DRIVER(4), BIT(5)},
	/* LED38 */	{DRIVER(4), BIT(6)},
	/* LED39 */	{DRIVER(4), BIT(7)},
	/* LED40 */	{DRIVER(5), BIT(0)},
	/* LED41 */	{DRIVER(5), BIT(1)},
	/* LED42 */	{DRIVER(5), BIT(2)},
	/* LED43 */	{DRIVER(5), BIT(3)},
	/* LED44 */	{DRIVER(5), BIT(4)},
	/* LED45 */	{DRIVER(5), BIT(5)},
	/* LED46 */	{DRIVER(5), BIT(6)},
	/* LED47 */	{DRIVER(5), BIT(7)},
	/* LED48 */	{DRIVER(6), BIT(0)},
	/* LED49 */	{DRIVER(6), BIT(1)},
	/* LED50 */	{DRIVER(6), BIT(2)},
	/* LED51 */	{DRIVER(6), BIT(3)},
	/* LED52 */	{DRIVER(6), BIT(4)},
	/* LED53 */	{DRIVER(6), BIT(5)},
	/* LED54 */	{DRIVER(6), BIT(6)},
	/* LED55 */	{DRIVER(6), BIT(7)},
	/* LED56 */	{DRIVER(7), BIT(0)},
	/* LED57 */	{DRIVER(7), BIT(1)},
	/* LED58 */	{DRIVER(7), BIT(2)},
	/* LED59 */	{DRIVER(7), BIT(3)},
	/* LED60 */	{DRIVER(7), BIT(4)},
	/* LED61 */	{DRIVER(7), BIT(5)},
	/* LED62 */	{DRIVER(7), BIT(6)},
	/* LED63 */	{DRIVER(7), BIT(7)},
};

//GPIO lábakat init-eli
static void gpio_init(void){

	//Grace inicializálja a lábakat
	//itt csak alapállapotba állítjuk a lábakat
	//jó lenne, ha már Grace alapállapotba tenné a lábakat
	SET_nOE();
	CLR_LE();
#if !TLC5916_USE_SPI
	CLR_SCK();
	CLR_SDI();
#endif	//TLC5916_USE_SPI
	
}



#if !TLC5916_USE_SPI

//egy bitet kiküld a ledmeghajtó SDI lábára
//b=0: low
//b!=0: high
static void send_one_bit(int b){
	
	if(b)	SET_SDI();
	else	CLR_SDI();
	
	SET_SCK();
	CLR_SCK();

}

//kiküldi a data-ban tárolt összes bitet
static void send(void){

	int i;
	int j;

	for(i=DRIVER_COUNT ; i-- > 0 ; )
		for(j=LED_PER_DRIVER ; j-- > 0 ; )
			//nem shiftelünk, csak kinézzük a táblázatból
			send_one_bit(data[i] & bit_table[j]);
			//send_one_bit(data[i] & BIT(j));

}

#else	//TLC5916_USE_SPI

//egy bájtot kiküld a ledmeghajtó SDI lábára
//MSB megy ki elõször
static void send_one_byte(uint8_t byte){

	USISRL = byte;				// Load shift register with data byte to be TXed
	USICNT = 8;					// Load bit-counter to send/receive data byte
	while(!(USIIFG & USICTL1));	// Loop until data byte transmitted
	byte = USISRL;				// Read out the received data

}

//kiküldi a data-ban tárolt összes bitet
static void send(void){

	int i;

	for(i=DRIVER_COUNT ; i-- > 0 ; )
		send_one_byte(data[i]);

}

#endif	//TLC5916_USE_SPI

//latch impulzust ad
static void latch(void){
	
	SET_LE();
	CLR_LE();

}

//egy LED kívánt állapotának megfelelõen beállítja a data[] tömb megfelelõ elemének egy bitjét
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

static void write_leds(void* states, int from_led, int led_count){

	const int size = 8;
	uint8_t* st = (uint8_t*)states;
	
	int b;

	if(from_led >= LED_COUNT)
		return;

	if(from_led + led_count > LED_COUNT)
		led_count = LED_COUNT - from_led;

	//led: from_led + b
	//on_off: st által mutatott terület (b/size)-adik bájtjának (b%size)-adik bitje
	for(b=0 ; b < led_count ; b++)
		//write_led(from_led + b, (*(st+(b/size))) & bit_table[b%size]);
		//size=8, így shiftelésre és maszkolásra cserélhetjük, mert a fordító nem veszi észre...
		write_led(from_led + b, (*(st+(b>>3))) & bit_table[b&0x07]);

}

//init-eli a változókat, a GPIO-kat, kikapcsolja az összes LED-et, majd engedélyezi a meghajtást
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
void tlc5916_write_leds(void* states, int from_led, int led_count){
	write_leds(states, from_led, led_count);
}
void tlc5916_init(void){
	init();
}
