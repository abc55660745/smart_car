#include "structoperation.h"
#include <string.h>
#include <stdlib.h>

struct anotext char2ano(char input[])
{
	uint8_t *point = (uint8_t*)input;
	struct anotext r;
	r.head = 0x0000;
	if(*point == 0xaa)
	{
		point++;
		if(*point == 0xaf)
		{
			//下面是功能字解析，现在就写了一个
			point++;
			if(*point == 0x10)
			{
				point++;
				r.data = malloc(*point);
				char2short((char*)point + 1, (int16_t*)r.data, *point / 2);
				r.head = 0xaaaf;
				r.name = 0x10;
				r.len = *point;
				r.sum = *(point + *point);
			}
		}
	}
	return r;
}

char* ano2char(struct anotext input)
{
	char *r = malloc(input.sum + 1), *t = r;
	for(;t - r < input.sum + 1;t++)
		*t = 0;
	char *point = r;
	t = (char*)&input.head;
	*point = *t;
	point++;
	t++;
	*point = *t;
	point++;
	*point = input.name;
	point++;
	*point = input.len;
	point++;
	strcpyn(point, input.data, input.len);
	r[input.sum - 1] = input.sum;
	return r;
}

void delstr(struct anotext input)
{
	free(input.data);
}	

void short2char(char* out, int16_t in[], uint8_t n)
{
	char *cpoint = out, *ipoint = (char*)in;
	for(;ipoint - (char*)in < n;cpoint++, ipoint++)
	{
		*cpoint = *ipoint;
	}
}

void strcpyn(char *dest, char *sour, uint8_t n)
{
	char *dp = dest, *sp = sour, temp;
	for(;dp - dest < n; dp++, sp++)
		*dp = *sp;
	for(sp = dest, dp--; dp - sp > 0; dp--, sp++)
	{
		temp = *dp;
		*dp = *sp;
		*sp = temp;
	}
}

void char2short(char* in, int16_t out[], uint8_t n)
{
	char *sp;
	int16_t *ip;
	for(sp = in, ip = out; sp >= in; sp += 2, ip++)
	{
		*ip = *(int16_t*)sp;
	}
}
