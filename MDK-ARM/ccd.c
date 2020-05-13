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

const float ccd_q[128] = {10.800000, 9.642857, 8.901099, 9.101124, 8.019802, 
								7.714286, 7.168142, 6.864407, 6.532258, 6.090226, 5.785714, 
								5.510204, 5.192308, 4.939024, 4.709302, 4.450549, 4.285714, 
								4.090909, 4.009901, 3.767442, 3.600000, 3.506494, 3.403361, 
								3.360996, 3.389121, 3.139535, 3.115385, 3.079848, 3.022388, 
								3.011152, 2.956204, 2.913669, 2.862191, 2.842105, 2.793103, 
								2.682119, 2.629870, 2.539185, 2.484663, 2.439759, 2.368421, 
								2.314286, 2.334294, 2.243767, 2.201087, 2.195122, 2.160000, 
								2.131579, 2.120419, 2.087629, 2.045455, 2.030075, 2.009926, 
								1.970803, 1.942446, 2.250000, 1.892523, 1.928571, 1.910377, 
								1.937799, 1.879350, 1.879350, 1.961259, 1.892523, 1.875000, 
								1.892523, 1.910377, 1.951807, 1.928571, 1.951807, 1.966019, 
								1.980440, 2.004950, 2.076923, 2.061069, 2.098446, 2.131579, 
								2.142857, 2.219178, 2.334294, 2.301136, 2.327586, 2.454545, 
								2.432432, 2.500000, 2.547170, 2.621359, 2.682119, 2.755102, 
								2.822300, 2.903226, 3.022388, 3.079848, 3.127413, 3.266129, 
								3.279352, 3.417722, 3.521739, 3.632287, 3.732719, 3.820755, 
								3.951220, 4.070352, 4.175258, 4.402174, 4.475138, 4.628571, 
								4.764706, 5.000000, 5.192308, 5.328947, 5.586207, 5.744681, 
								6.000000, 6.230769, 6.428571, 6.585366, 6.750000, 6.923077, 
								7.105263, 7.297297, 7.714286, 7.788462, 8.181818, 8.526316, 
								9.000000, 9.529412, 9.87804};

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
		//yuzhi += (max - min) / 3;
		
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
	
	if(temp != 255)
	{
		t = (float)temp * ccd_q[ccd_count - 1] + 20;
		if(t <= 255)
			ccd_s[ccd_count - 1] = t;
		else
			ccd_s[ccd_count - 1] = 255;
	}
	else
		ccd_s[ccd_count - 1] = temp;
	
	//ccd_s[ccd_count - 1] = temp;
}

//通过算术平均滤波算法生成pwm值
uint16_t gen_pwm()
{
	/*
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
	*/
	uint8_t temp;
	temp = 23 + (double)(direction[0]) * 0.7;
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
	uint8_t i, count = 0, line[32] = {0}, down, down_ok = 0, flag = 0;
	for(i = 16; i <= 112; i++)
	{
		if(!down_ok)
		{
			if(ccd_p[i - 1] && ccd_p[i] && !ccd_p[i + 1] && !ccd_p[i + 2])
			{
				down = i;
				down_ok = 1;
			}
		}
		else
		{
			if(!ccd_p[i - 1] && !ccd_p[i] && ccd_p[i + 1] && ccd_p[i + 2] && i - down < 18)
			{
				line[count] = (i + down) / 2;
				down_ok = 0;
			}
		}
	}
	for(i = 0; i < 31 && line[i + 1] && line[i]; i++)
	{
		if(!ren_flag)
		{
			if(line[i + 1] - line[i] < 20)
				flag = 1;
		}
		else
		{
			if(line[i + 1] - line[i] < 50)
				flag = 1;
		}
	}
	ren_flag = flag;
}
