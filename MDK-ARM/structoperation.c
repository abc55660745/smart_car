#include "structoperation.h"
#include <string.h>
#include <stdlib.h>

struct anotext char2ano(char input[])
{
	char *point = input, *temp;
	struct anotext r;
	uint8_t sum;
	r.head = *(uint16_t*)point;
	point += 2;
	r.name = *(uint8_t*)point;
	point++;
	r.len = *(uint8_t*)point;
	point++;
	sum = strlen(input);
	r.sum = input[sum - 1];
	if(r.sum != sum || r.sum - 5 != r.len)
	{
		r.len = 0;
		r.sum = 0;
		r.data = NULL;
		return r;
	}
	r.data = malloc(r.len + 1);
	for(temp = r.data; temp - r.data < r.len; temp++, point++)
	{
		*temp = *point;
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


