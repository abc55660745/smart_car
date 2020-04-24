#ifndef STRUCT_INIT
#define STRUCT_INIT

#include "stdint.h"

struct anotext{
	uint16_t head;
	uint8_t name;
	uint8_t len;
	char data[251];
	uint8_t sum;
};

struct anotext char2ano(char[]);
char* ano2char(struct anotext);
void rxprocess(char[]);

#endif