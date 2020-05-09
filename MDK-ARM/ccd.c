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

const float ccd_q[64] = {9.929103856, 9.492198283, 9.074517656, 8.675216028,
	8.293484677, 7.928550467, 7.579674281, 7.246149525, 6.927300699, 6.622482024,
	6.331076139, 6.052492846, 5.78616792, 5.531561961, 5.288159305, 5.055466979,
	4.833013701, 4.620348928, 4.417041941, 4.222680972, 4.036872376, 3.859239826,
	3.689423556, 3.527079629, 3.371879243, 3.223508065, 3.081665593, 2.946064547,
	2.816430288, 2.692500263, 2.574023472, 2.460759957, 2.352480322, 2.248965264,
	2.150005129, 2.05539949, 1.964956736, 1.878493692, 1.795835239, 1.716813966,
	1.641269828, 1.569049823, 1.50000768, 1.434003564, 1.370903797, 1.310580577,
	1.252911732, 1.19778046, 1.145075104, 1.094688916, 1.046519847, 1.000470338,
	0.956447124, 0.914361042, 0.874126854, 0.835663071, 0.798891792, 0.763738541,
	0.730132122, 0.69800447, 0.667290516, 0.637928053, 0.609832773, 0.582998596};

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
	else if (htim == (&htim7))
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
	uint8_t temp;
	uint16_t t;
	ad_flag = 1;  //ADC同步锁解锁
	HAL_ADC_Stop_IT(&hadc2); 
	temp = HAL_ADC_GetValue(&hadc2);  //将ADC数据写入CCD数组
	/*
	if(temp != 255)
	{
		if(ccd_count <= 64)
			t = (float)temp * ccd_q[ccd_count - 1];
		else
			t = (float)temp * ccd_q[128 - ccd_count];
		if(t <= 255)
			ccd_s[ccd_count - 1] = t;
		else
			ccd_s[ccd_count - 1] = 255;
	}
	else
		ccd_s[ccd_count - 1] = temp;
	*/
	ccd_s[ccd_count - 1] = temp;
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
