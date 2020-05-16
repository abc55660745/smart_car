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
uint8_t ren_flag_count = 0;
uint8_t ren_dir[2] = {0};
uint8_t process_flag = 1;
uint8_t step = 0;

const float ccd_q[128] = {2.621359, 2.347826, 2.237569, 2.347826, 2.137203, 
								2.137203, 2.055838, 2.040302, 1.985294, 1.923990, 1.905882, 
								1.875000, 1.840909, 1.796009, 1.776316, 1.741935, 1.719745, 
								1.683992, 1.691023, 1.636364, 1.600791, 1.572816, 1.566731, 
								1.563707, 1.591356, 1.494465, 1.483516, 1.475410, 1.451613, 
								1.446429, 1.423550, 1.403813, 1.398964, 1.396552, 1.379898, 
								1.347754, 1.341060, 1.312804, 1.300161, 1.291866, 1.269592, 
								1.246154, 1.275591, 1.232877, 1.219880, 1.219880, 1.207154, 
								1.192931, 1.187683, 1.180758, 1.155492, 1.158798, 1.148936, 
								1.136045, 1.125000, 1.285714, 1.108071, 1.126565, 1.118785, 
								1.136045, 1.105048, 1.117241, 1.160458, 1.118785, 1.114168, 
								1.121884, 1.123440, 1.140845, 1.129707, 1.131285, 1.131285, 
								1.132867, 1.137640, 1.167147, 1.157143, 1.165468, 1.173913, 
								1.170520, 1.194690, 1.242331, 1.208955, 1.212575, 1.259720, 
								1.244240, 1.265625, 1.271586, 1.281646, 1.296000, 1.310680, 
								1.308562, 1.325696, 1.347754, 1.361345, 1.361345, 1.389365, 
								1.372881, 1.406250, 1.418564, 1.438721, 1.441281, 1.441281, 
								1.456835, 1.472727, 1.483516, 1.502783, 1.514019, 1.539924, 
								1.551724, 1.591356, 1.610338, 1.616766, 1.633065, 1.659836, 
								1.708861, 1.760870, 1.800000, 1.857798, 1.896956, 1.956522, 
								1.970803, 2.014925, 2.093023, 2.125984, 2.177419, 2.237569, 
								2.320917, 2.454545, 2.53918};

void ccd_process(void);
uint16_t gen_pwm(void);
void line_go(void);
void ren_go(void);
void ren_judge(void);
								
int16_t sabs(int16_t in)
{
		if(in >= 0)
			return in;
		else
		{
			int16_t t = in * -1;
			return t;
		}
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
	  //这是新的识路程序，效果海星
		uint8_t left = direction[1], right = direction[1], temp, flag = 0, i;
		uint16_t speed;
		if(ccd_ok)
		{
			ccd_process();
			for(i = 4; i < 124; i++)
			{
				if(!ccd_p[0][i])
					flag = 1;
			}
			if(1)
			{
				if(!ccd_p[0][63])
				{
					while(left > 13 && !(!ccd_p[0][left] && !ccd_p[0][left - 1]
							&& !ccd_p[0][left - 2] && !ccd_p[0][left - 3]))
						left--;
					while(right < 115 && !(!ccd_p[0][right] && !ccd_p[0][right + 1]
							&& !ccd_p[0][right + 2] && !ccd_p[0][right + 3]))
						right++;
				}
				ren_judge();
				if((ren_flag || ren_count) && ren_count <= 40000)
				{
					ren_count++;
				}
				if(ren_count > 0 && ren_count < 60000)
				{
					ren_go();
				}
				else// if(right - left < 30)
				{
					line_go();
				}
				temp = gen_pwm();
				__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, temp);
			}
			//这里是自动调速程序
			
			if(step == 0)
				speed = 110 - sabs(direction[0] - 63) / 2 - sabs(direction[1] - direction[0]);
			else if(step == 1)
				speed = 60;
			else
				speed = 40;
			//speed = 75;
			__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_1, speed);
			__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, speed);
			
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
		//process_flag = 1;
	}
	else
	{
		for(i = 0; i < 128; i++)
			ccd_p[0][i] = 1;
		//process_flag = 0;
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
	int8_t i;
	
	//由于效果原因该滤波暂不使用
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
	temp = 30 + (double)(sum / count) * 0.6;
	return temp;
	
	/*
	uint8_t temp;
	
	for(i = MAXN - 2; i >= 0 ; i--)
	{
		direction[i + 1] = direction[i];
	}
	
	
	temp = 30 + (double)(direction[0]) * 0.6;
	return temp;
	*/
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
	if(ren_dir[1])
	{
		if(ren_count < 20000)  //这个值还得调
		{
			//向左转
			direction[0] = ren_dir[1];
		}
		else
		{
			//向右转
			//direction[0] = ren_dir[0];
			line_go();
		}
	}
	else
		//direction[0] = ren_dir[0];
	line_go();
	if(ren_flag_count > 3)
		direction[0] = ren_dir[0];
}

void ren_judge()
{
	uint8_t i, count = 0, line[32] = {0}, down, down_ok = 0, flag = 0;
	for(i = 16; i <= 112; i++)
	{
		if(!down_ok)
		{
			if(ccd_p[0][i - 1] && ccd_p[0][i] && !ccd_p[0][i + 1] && !ccd_p[0][i + 2])
			{
				down = i;
				down_ok = 1;
			}
		}
		else
		{
			if(!ccd_p[0][i - 1] && !ccd_p[0][i] && ccd_p[0][i + 1] && ccd_p[0][i + 2] && i - down < 18)
			{
				line[count] = (i + down) / 2;
				down_ok = 0;
				count++;
			}
		}
	}
	for(i = 0; i < 31 && line[i + 1] && line[i]; i++)
	{
		if(!ren_flag)
		{
			if(line[i + 1] - line[i] < 20)
			{
				flag = 1;
				ren_dir[0] = line[i];
				ren_dir[1] = line[i + 1];
			}
		}
		else
		{
			if(line[i + 1] - line[i] < 50)
			{
				flag = 1;
				ren_dir[0] = line[i];
				ren_dir[1] = line[i + 1];
			}
		}
	}
	ren_dir[0] = line[0];
	ren_dir[1] = line[1];
	for(i = 0; i < 32; i++)
		line[i] = 0;
	if(ren_flag == 0 && flag == 1)
		ren_flag_count++;
	ren_flag = flag;
}
