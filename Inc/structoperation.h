#ifndef STRUCT_INIT
#define STRUCT_INIT

#include "stdint.h"

struct anotext{
	uint16_t head;
	uint8_t name;
	uint8_t len;
	char *data;
	uint8_t sum;
};

struct anotext char2ano(char[]);
char* ano2char(struct anotext);
void short2char(char*, int16_t[], uint8_t);
void rxprocess(char[]);
void delstr(struct anotext);
void strcpyn(char*, char*, uint8_t);

#endif
