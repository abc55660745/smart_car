#ifndef CONNECT
#define CONNECT

#include "main.h"

//将CCD原始数据发送至CCD上位机软件
void send_ccd(void);

//通过串口2向匿名地面站发送函数，参数1为发送数组指针，参数2为发送数组大小
void send(int16_t *in, uint8_t n);

#endif
