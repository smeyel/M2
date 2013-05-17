
#include "tlc5916.h"
#include "time.h"

int main(void){

	tlc5916_init();
	time_init();

	#if 1
	tlc5916_write_led(0, 1);
	tlc5916_write_led(2, 1);
	tlc5916_write_led(4, 1);
	tlc5916_send();
	tlc5916_latch();
	#endif
	
	while(1);

	return 0;

}
