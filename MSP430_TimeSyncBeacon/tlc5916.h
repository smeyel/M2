#ifndef TLC5916_H_
#define TLC5916_H_

void tlc5916_send(void);
void tlc5916_latch(void);
void tlc5916_write_led(int led, int on_off);
void tlc5916_write_leds(void* states, int from_led, int led_count);
void tlc5916_init(void);

#endif //TLC5916_H_
