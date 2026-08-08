#include "HX711.h"
/*****************称重模块引脚定义*******************/
sbit ADDO = P3^2;
sbit ADSK = P3^4;
unsigned long ReadWeight(void)//读取称重传感器函数
{
	unsigned long Count=0;
	unsigned char i=0;
	ADSK=0; //使能AD（PD_SCK 置低）
	Count=0;
	while(ADDO); //AD转换未结束则等待，否则开始读取
	for (i=0;i<24;i++)
	{
		ADSK=1; //PD_SCK 置高（发送脉冲）
		Count=Count<<1; //下降沿来时变量Count左移一位，右侧补零
		ADSK=0; //PD_SCK 置低
		if(ADDO) Count++;
	}
	ADSK=1;
	Count=Count^0x800000;//第25个脉冲下降沿来时，转换数据
	ADSK=0;
	return(Count);
}