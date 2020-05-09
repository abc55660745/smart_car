#include "main.h"
#include "adc.h"
#include "tim.h"

extern char ccd_flag;
extern uint16_t ccd_count;
extern char ad_flag;
extern uint8_t ccd_s[128];
extern uint8_t ccd_ok;
extern uint16_t ccd_SI;
extern int16_t direction[MAXN];
extern uint8_t ccd_p[2][128];
uint8_t ren_flag = 0;
uint16_t ren_count = 0;

void ccd_process(void);
uint16_t gen_pwm(void);
void line_go(void);
void ren_go(void);
void ren_judge(void);

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
			else if(ccd_count == 130)  //计数130时ADC转换全部完成，解锁下一步处理
			{
				ccd_ok = 1;
			}
			if(ccd_count == ccd_SI)  //判断是否达到曝光时间启动下一个SI循环
			{
				ccd_count = 0;
				ccd_ok = 0;
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
	    //这是新的识路程序，效果待测
		uint8_t left = direction[1], right = direction[1], temp;
		if(ccd_ok)
		{
			ccd_process();
			if(!ccd_p[0][63])
			{
				while(left > 13 && !(!ccd_p[0][left] && !ccd_p[0][left - 1]
						&& !ccd_p[0][left - 2] && !ccd_p[0][left - 3]))
					left--;
				while(right < 115 && !(!ccd_p[0][right] && !ccd_p[0][right + 1]
						&& !ccd_p[0][right + 2] && !ccd_p[0][right + 3]))
					right++;
			}
			if(right - left < 30)
			{
				ren_judge();
				if((ren_flag || ren_count) && ren_count <= 60000)
				{
					ren_count++;
				}
				if(ren_flag)
				{
					ren_go();
				}
				else
				{
					line_go();
				}
				temp = gen_pwm();
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, temp);
			}
		}
    }
}

//CCD数据处理函数
void ccd_process()
{
	uint8_t max, min, i = 5, yuzhi;
	
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
}

//ADC中断回调函数
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	ad_flag = 1;  //ADC同步锁解锁
	 HAL_ADC_Stop_IT(&hadc2); 
	ccd_s[ccd_count - 1] = HAL_ADC_GetValue(&hadc2);  //将ADC数据写入CCD数组
}

//通过算术平均滤波算法生成pwm值
uint16_t gen_pwm()
{
	int8_t i;
	uint8_t count = 0;
	uint16_t temp, sum = 0;
	for(i = 0; i < MAXN && direction[i]; i++)
	{
		sum += direction[i];
		count++;
	}
	for(i = MAXN - 2; i >= 0 ; i--)
	{
		direction[i + 1] = direction[i];
	}
	temp = 23 + (double)(sum / count) * 0.7;
	return temp;
}

void line_go()
{
	uint8_t left = 66, right = 60;
	while(left > 13 && !(!ccd_p[0][left] && !ccd_p[0][left - 1]
			&& !ccd_p[0][left - 2] && !ccd_p[0][left - 3]))
		left--;
	while(right < 115 && !(!ccd_p[0][right] && !ccd_p[0][right + 1]
			&& !ccd_p[0][right + 2] && !ccd_p[0][right + 3]))
		right++;
	left--;
	right++;
	if(63 - left >= right - 63)
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
}

void ren_go()
{
	if(ren_count < 1000)  //这个值还得调
	{
		//向左转
	}
	else
	{
		//向右转
	}
}

void ren_judge()
{
	
}
