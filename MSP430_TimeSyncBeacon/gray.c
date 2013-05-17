
#include "gray.h"

uint32_t binaryToGray(uint32_t num){
	return (num >> 1) ^ num;
}
 
uint32_t grayToBinary(uint32_t num){
	uint32_t mask;
	for(mask = num >> 1; mask != 0; mask = mask >> 1)
		num = num ^ mask;
	return num;
}
