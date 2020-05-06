/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "structoperation.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define RXBUFFERSIZE  256

char RxBuffer[RXBUFFERSIZE];  //串口接收使用数组
uint8_t aRxBuffer;            //同上
uint8_t Uart1_Rx_Cnt = 0;     //串口接收计数
int16_t direction[3] = {0};       //舵机PID值暂存
char ad_flag = 1;             //ADC完成标志位（用于同步锁）
char ccd_flag = 0;            //CCD的CLK电平记录，用于调控CLK输出
char zhijiao = 0;             //暂时没啥用
uint8_t ccd_s[128] = {0};     //CCD原始值记录
uint8_t ccd_p[2][128] = {0};  //CCD处理值记录，二维数组保存上一次记录
uint16_t ccd_count = 0;       //CCD的CLK输出计次，用于调控数组写入
uint16_t ccd_SI = 1200;       //CCD曝光时间，单位为半个CLK周期

//通过串口2向匿名地面站发送函数，参数1为发送数组指针，参数2为发送数组大小
void send(int16_t*, uint8_t);

//ccd处理函数，包含二值化和舵机控制
void ccd_process(void);

//将CCD原始数据发送至CCD上位机软件
void send_ccd(void);

//CCD上位机通信使用
void PutChar(unsigned char data);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_TIM7_Init();
  MX_TIM1_Init();
  MX_TIM8_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
	//定时器和串口初始化
	HAL_UART_Receive_IT(&huart1, (uint8_t *)&aRxBuffer, 1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
	HAL_TIM_Base_Start_IT(&htim4);
	HAL_TIM_Base_Start_IT(&htim7);
	
	//写入IO口初始值
	HAL_GPIO_WritePin(SI_GPIO_Port, SI_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PWM2_GPIO_Port, PWM2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PWM2_GPIO_Port, PWM4_Pin, GPIO_PIN_RESET);
	
	//设置电机PWM（暂时没写差速
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, 70);
	__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, 70);
	//printf("start\n");
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		//printf("test");
		/* 把这个留在这里，ADC启动转换使用
		if(ad_flag)
		{
			HAL_ADC_Start_IT(&hadc2);
			ad_flag = 0;
		}
		*/
		//int16_t data[2] = {10,20};
		//data[0] = direction;
		//send(data, 2);
		/*
		//__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, direction);
		while (direction< 113)
	  {
			
		  direction++;
			int16_t data[2] = {10,20};
			data[0] = direction;
			send(data, 2);
		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, direction);    //?????,?????
//		  TIM3->CCR1 = pwmVal;    ?????
		  HAL_Delay(20);
	  }
	  while (direction > 23)
	  {
		  direction--;
			int16_t data[2] = {10,20};
			data[0] = direction;
			send(data, 2);
		  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, direction);    //?????,?????
//		  TIM3->CCR1 = pwmVal;     ?????
		  HAL_Delay(20);
		}*/
		
		//通过串口3直接以01发送CCD二值化以后的数据
		char se[130];
		uint8_t i;
		HAL_Delay(500);
		for(i = 0; i < 128; i++)
		{
			se[i] = ccd_p[0][i] + '0';
		}
		se[128] = '\n';
		se[129] = '\r';
		HAL_UART_Transmit(&huart3, (uint8_t *)direction[1], 1,0xFFFF);
		//HAL_UART_Transmit(&huart3, (uint8_t *)se, 130,0xFFFF);
		
		
		
		//HAL_Delay(50);
		send_ccd();
		//printf("\ntt\n");
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

//串口回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
	struct anotext send;
	char ss[2], *sss;
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
 
	if(Uart1_Rx_Cnt >= 255)
	{
		Uart1_Rx_Cnt = 0;
		memset(RxBuffer,0x00,sizeof(RxBuffer));
		HAL_UART_Transmit(&huart1, (uint8_t *)"数据溢出", 10,0xFFFF); 	
        
	}
	else
	{
		RxBuffer[Uart1_Rx_Cnt++] = aRxBuffer;
		//匿名地面站数据解析程序
		struct anotext t = char2ano(RxBuffer);
		if(t.head == 0xaaaf && t.sum == Uart1_Rx_Cnt)
		{
			uint8_t i;
			send.head = 0xaaaa;
			send.name = 0xef;
			send.len = 2;
			ss[0] = t.name;
			ss[1] = t.sum;
			send.data = ss;
			send.sum = 0;
			sss = ano2char(send);
			for(i = 0; i < 6; i++)
				sss[6] = sss[i];
			HAL_UART_Transmit(&huart1, (uint8_t *)sss, 7,0xFFFF);
			free(sss);
			delstr(send);
			//写入数据部分写在下面
			
		}
	}
	
	HAL_UART_Receive_IT(&huart1, (uint8_t *)&aRxBuffer, 1);
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


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == (&htim4))
  {
	//定时器4中断函数
		if(ccd_flag)  //CLK为1时将CLK写0，并将CCD计数+1
		{
			HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
			ccd_flag = 0;
			ccd_count++;
			if(ccd_count <= 128)  //在CCD下降沿启动ADC转换
			{
				if(ad_flag)  //根据ADC是否上锁决定是否调用ADC
				{
					HAL_ADC_Start_IT(&hadc2);
					ad_flag = 0;  //ADC上锁（转换完成后解锁
				}
				else
				{
					ccd_s[ccd_count - 1] = 127;  //如果ADC上锁则直接写入中值
				}
			}
			else if(ccd_count == 130)  //计数130时ADC转换全部完成，调用下一步处理程序
			{
				ccd_process();
			}
			if(ccd_count == ccd_SI)  //判断是否达到曝光时间启动下一个SI循环
			{
				ccd_count = 0;
				HAL_GPIO_WritePin(SI_GPIO_Port, SI_Pin, GPIO_PIN_SET);
			}
			if(ccd_count == 1)  //SI写0
			{
				HAL_GPIO_WritePin(SI_GPIO_Port, SI_Pin, GPIO_PIN_RESET);
			}
		}
		else  //CLK为0时写1
		{
			HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);
			ccd_flag = 1;
		}
	
  }
		if (htim == (&htim7))
    {
      //定时器7中断函数
			/* 本段代码已废弃，这里以后要写PID程序
			uint8_t count = 0, i;
			int16_t sum = 0;
			for(i = 5; i < 123; i++)
			{
				if(!ccd_p[i])
				{
					sum += i;
					count++;
				}
			}
			if(count < 10)
			{
				sum /= count;
				sum -= 63;
				direction = sum * 0.7 + 68;
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, direction);
			}
			*/
		  //这是新的识路程序，效果待测
			uint8_t left = 63, right = 63, temp, dir;
			if(!ccd_p[0][63])
			{
				while(left > 13 && !(ccd_p[0][left] && ccd_p[0][left - 1]
						&& ccd_p[0][left - 2] && ccd_p[0][left - 3]))
					left--;
				while(right < 115 && !(ccd_p[0][right] && ccd_p[0][right + 1]
						&& ccd_p[0][right + 2] && ccd_p[0][right + 3]))
					right++;
			}
			if(right - left < 14)
			{
				left = 66;
				right = 60;
				while(left > 13 && !(!ccd_p[0][left] && !ccd_p[0][left - 1]
						&& !ccd_p[0][left - 2] && !ccd_p[0][left - 3]))
					left--;
				while(right < 115 && !(!ccd_p[0][right] && !ccd_p[0][right + 1]
						&& !ccd_p[0][right + 2] && !ccd_p[0][right + 3]))
					right++;
				left--;
				right++;
				if(abs(left - 63) >= abs(right - 63))
					left = right;
				else
					right = left;
				while(left > 13 && !(ccd_p[0][left] && ccd_p[0][left - 1]
						&& ccd_p[0][left - 2] && ccd_p[0][left - 3]))
					left--;
				while(right < 115 && !(ccd_p[0][right] && ccd_p[0][right + 1]
						&& ccd_p[0][right + 2] && ccd_p[0][right + 3]))
					right++;
				direction[0] = (right + left) / 2;
				if(direction[1] != 0)
					temp = (direction[0] + direction[1]) / 2;
				direction[1] = direction[0];
				temp = 23 + (double)temp * 0.7;
				if(direction[0] > 30 && direction[0] < 106)
					__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, temp);
				else
					direction[1] = 0;
			}
    }
}

//ADC中断回调函数
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	ad_flag = 1;  //ADC同步锁解锁
	 HAL_ADC_Stop_IT(&hadc2); 
	ccd_s[ccd_count - 1] = HAL_ADC_GetValue(&hadc2);  //将ADC数据写入CCD数组
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

//CCD数据处理函数
void ccd_process()
{
	uint8_t max, min, i = 5, j, yuzhi, count = 0, temp[128], ok = 0;
	int16_t sum = 0;
	
	//循环寻找CCD原始数据中最大值与最小值
	for(max = ccd_s[i], min = ccd_s[i]; i < 123; i++)
	{
		if(ccd_s[i] > max)
			max = ccd_s[i];
		if(ccd_s[i] < min)
			min = ccd_s[i];
	}
	if(max - min > 5)
	{
		yuzhi = (max + min) / 2;  //阈值设为最大值与最小值的中值
		yuzhi += (max - min) / 3;
		
		//对原始数据进行二值化处理
		for(i = 0; i < 128; i++)
		{
			if(ccd_s[i] < yuzhi)
				ccd_p[0][i] = 0;
			else
				ccd_p[0][i] = 1;
		}
	}
	else
	{
		for(i = 0; i < 128; i++)
			ccd_p[0][i] = 1;
	}
	
	/*
	for(i = 0; i < 128; i++)
	{
		temp[i] = 0;
	}
	
	//下面使用两次识别取并集降低干扰
	//对本次数据与上次数据寻找并集，并将并集存在位置写入临时函数
	for(i = 4, count = 4; i < 124; i++)
	{
		if(ccd_p[0][i] == ccd_p[1][i] && ccd_p[0][i] == 0)
		{
			temp[count] = i;
			count++;
			ok = 1;
		}
	}
	
	//将i,j设为并集中位数位置
	i = temp[count / 2];
	j = i;
	count = 0;
	//从并集中位数位置开始，在本次数据中向上寻找上升沿
	for(;!ccd_p[0][i] && i <= 124; i++)
	{
		sum += i;
		count++;
	}
	//从并集中位数位置开始，在本次数据中向下寻找下降沿
	for(;!ccd_p[0][j] && j >= 4; j--)
	{
		sum += i;
		count++;
	}
	//上述操作得到上次识别的黑线在本次识别中的位置（去除误识别的其他黑色
	sum /= count;
	sum -= 63;
	//得到本次黑线的中间位置，使用简单计算换算为舵机PWM占空比并写入舵机
	direction = sum * 0.6 + 68;
	if(ok && direction != 0)
	{
		__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, direction);
	}
	for(i = 0; i < 128; i++)
	{
		ccd_p[1][i] = ccd_p[0][i];  //将本次数据写入到上次位置，等待下一次读取
	}
	*/
	
}

//重定义printf使用
int fgetc(FILE *f)
{
  uint8_t ch = 0;
  HAL_UART_Receive(&huart1, &ch, 1, 0xffff);
  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
