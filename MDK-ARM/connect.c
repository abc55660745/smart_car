#include "connect.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include "structoperation.h"

void SendHex(unsigned char data);
void PutChar(unsigned char data);

extern uint8_t ccd_s[128];

//CCD上位机通信使用
void SendHex(unsigned char data)
{
	unsigned char temp;
	temp = data >> 4;
	if(temp >= 10)
	{
		PutChar(temp - 10 + 'A');
	}
	else
	{
		PutChar(temp + '0');
	}
	temp = data & 0x0f;
	if(temp >= 10)
	{
		PutChar(temp - 10 + 'A');
	}
	else
	{
		PutChar(temp + '0');
	}
}

//向CCD上位机发送CCD原始数据
void send_ccd()
{
	uint8_t dataa[150], *data = dataa;
	int len;
	unsigned char lrc=0;
	dataa[0] = 0;
	dataa[1] = 132;
	dataa[2] = 0;
	dataa[3] = 0;
	dataa[4] = 0;
	dataa[5] = 0;
	for(len = 0; len < 128; len++)
	{
		dataa[len + 6] = ccd_s[len];
	}
	PutChar('*'); // 发送帧头，一个字节
	len = (int)(data[0]<<8) | (int)(data[1]) ;
	data += 2; // 调整指针
	PutChar('L'); // 发送帧类型，共两个字节
	PutChar('D');
	while(len--) // 发送数据的ASCII码，含保留字节和CCD数据
	{
		SendHex(*data);
		lrc += *data++;
	}
	lrc = 0-lrc; // 计算CRC，可以为任意值
	SendHex(lrc); // 发送CRC校验ASCII
	PutChar('#'); // 发送帧尾，一个字节
}



//向匿名地面站发送数据，使用方法见上，需要包含structoperation
void send(int16_t *in, uint8_t n)
{
	uint8_t i;
	struct anotext send;
	send.head = 0xaaaa;
	send.name = 0xf3;
	send.len = n * 2;
	send.data = malloc(n * 2 + 1);
	send.data[n * 2] = 0;
	short2char(send.data, in, n * 2);
	send.sum = 0;
	char *ss = ano2char(send);
	for(i = 0; i < n * 2 + 4; i++)
		ss[n * 2 + 4] += ss[i];
	HAL_UART_Transmit(&huart2, (uint8_t *)ss, n * 2 + 5,0xFFFF);
	free(ss);
	delstr(send);
}


//CCD上位机通信使用
void PutChar(unsigned char data)
{
	unsigned char *p = &data;
	HAL_UART_Transmit(&huart1, (uint8_t *)p, 1, 0xffff);
}

//重定义printf使用
int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xffff);
  return ch;
}

//重定义printf使用
int fgetc(FILE *f)
{
  uint8_t ch = 0;
  HAL_UART_Receive(&huart1, &ch, 1, 0xffff);
  return ch;
}
