#include "delay.h"
#include "usart.h"
#include "rtc.h" 		    
#include "datetime.h"

extern BACNET_DATE Local_Date;
extern BACNET_TIME Local_Time;
//			vu8 sec;				/* 0-59	*/
//			vu8 min;    		/* 0-59	*/
//			vu8 hour;      		/* 0-23	*/
//			vu8 day;       		/* 1-31	*/
//			vu8 week;  		/* 0-6, 0=Sunday	*/
//			vu8 mon;     		/* 0-11	*/
//			vu16 year;      		/* 0-99	*/
//			vu16 day_of_year; 	/* 0-365	*/
//			vu8 is_dst;        /* daylight saving time on / off */

extern U16_T Test[50];
UN_Time Rtc;//ʱ�ӽṹ�� 
/*
void set_clock(u16 divx)
{
 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��  
	PWR_BackupAccessCmd(ENABLE);	//ʹ��RTC�ͺ󱸼Ĵ������� 

	RTC_EnterConfigMode();/// ��������	
 
	RTC_SetPrescaler(divx); //����RTCԤ��Ƶ��ֵ          									 
	RTC_ExitConfigMode();//�˳�����ģʽ  				   		 									  
	RTC_WaitForLastTask();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������		 									  
}	   
*/

static void RTC_NVIC_Config(void)
{	
//	NVIC_InitTypeDef NVIC_InitStructure;
//	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;				//RTCȫ���ж�
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�1λ,�����ȼ�3λ
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//��ռ���ȼ�0λ,�����ȼ�4λ
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//ʹ�ܸ�ͨ���ж�
//	NVIC_Init(&NVIC_InitStructure);								//����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
}

//ʵʱʱ������
//��ʼ��RTCʱ��,ͬʱ���ʱ���Ƿ�������
//BKP->DR1���ڱ����Ƿ��һ�����õ�����
//����0:����
//����:�������
//void watchdog(void);
//void RTC_Check_Initial(void)
//{
//	uint8 rtc_state = 0;
//	uint8 i;
//	
//	rtc_state = 1;
//	while(rtc_state)
//	{ 
//		watchdog();
//		if(i < 20)
//		{
//			if(RTC_Init() == 1) //initial OK
//			{
//				rtc_state = 0;
//				break;
//			}
//			else
//				i++;
//		}
//		else
//		{
//			rtc_state = 0;
//			break;
//		}
//		delay_ms(100);	
//	}	
//	
//}

u8 RTC_Init(void)
{
	//����ǲ��ǵ�һ������ʱ��
	u8 temp = 0;	
	if(BKP_ReadBackupRegister(BKP_DR1) != 0x5050)	//��ָ���ĺ󱸼Ĵ����ж�������:��������д���ָ�����ݲ����
	{	 
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��   
		PWR_BackupAccessCmd(ENABLE);												//ʹ�ܺ󱸼Ĵ������� 
		BKP_DeInit();				//��λ�������� 	
		RCC_LSEConfig(RCC_LSE_ON);	//�����ⲿ���پ���(LSE),ʹ��������پ���
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	//���ָ����RCC��־λ�������,�ȴ����پ������
		{
			temp++;
			delay_ms(10);
			if(temp >= 250)
				return 0;	//��ʼ��ʱ��ʧ��,����������
		}
		
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);	//����RTCʱ��(RTCCLK),ѡ��LSE��ΪRTCʱ��    
		RCC_RTCCLKCmd(ENABLE);					//ʹ��RTCʱ��  
		RTC_WaitForLastTask();					//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		RTC_WaitForSynchro();					//�ȴ�RTC�Ĵ���ͬ��  
		RTC_ITConfig(RTC_IT_SEC, ENABLE);		//ʹ��RTC���ж�
		RTC_WaitForLastTask();					//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		RTC_EnterConfigMode();					//��������	
		RTC_SetPrescaler(32767);				//����RTCԤ��Ƶ��ֵ
		RTC_WaitForLastTask();					//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		Rtc_Set(16, 8, 27, 15, 42, 55,0);		//����ʱ��	
		RTC_ExitConfigMode(); 
		//�˳�����ģʽ  
		BKP_WriteBackupRegister(BKP_DR1, 0X5050);//��ָ���ĺ󱸼Ĵ�����д���û���������
	}
	else//ϵͳ������ʱ
	{
		RTC_WaitForSynchro();					//�ȴ����һ�ζ�RTC�Ĵ�����д�������
		RTC_ITConfig(RTC_IT_SEC, ENABLE);		//ʹ��RTC���ж�
		RTC_WaitForLastTask();					//�ȴ����һ�ζ�RTC�Ĵ�����д�������
	}
	
	RTC_NVIC_Config();							//RCT�жϷ�������		    				     
	RTC_Get();									//����ʱ��	
	return 1;
}

//RTCʱ���ж�
//ÿ�봥��һ��  
//extern u16 tcnt; 
//void RTC_IRQHandler(void)
//{		 
//	if(RTC_GetITStatus(RTC_IT_SEC) != RESET)	//�����ж�
//	{							
//		RTC_Get();//����ʱ��   
// 	}
//	
//	if(RTC_GetITStatus(RTC_IT_ALR)!= RESET)		//�����ж�
//	{
//		RTC_ClearITPendingBit(RTC_IT_ALR);		//�������ж�	  	   
//  	}
//	
//	RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);//�������ж�
//	RTC_WaitForLastTask();	  	    						 	   	 
//}

//�ж��Ƿ������꺯��
//�·�   1  2  3  4  5  6  7  8  9  10 11 12
//����   31 29 31 30 31 30 31 31 30 31 30 31
//������ 31 28 31 30 31 30 31 31 30 31 30 31
//����:���
//���:������ǲ�������.1,��.0,����
u8 Is_Leap_Year(u16 year)
{			  
	if(year % 4 == 0)				//�����ܱ�4����
	{ 
		if(year % 100 == 0) 
		{ 
			if(year % 400 == 0)		//�����00��β,��Ҫ�ܱ�400����
				return 1; 	   
			else
				return 0;   
		}
		else
		{
			return 1;   
		}
	}
	else 
	{
		return 0;
	}		
}

//����ʱ��
//�������ʱ��ת��Ϊ����
//��1970��1��1��Ϊ��׼
//1970~2099��Ϊ�Ϸ����
//����ֵ:0,�ɹ�;����:�������.
//�·����ݱ�											 
u8 const table_week[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};	//���������ݱ�	  
//ƽ����·����ڱ�

const u8 mon_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
u32 Rtc_Set(u16 syear, u8 smon, u8 sday, u8 hour, u8 min, u8 sec, u8 flag)
{
	u16 t;
	u32 seccount = 0;
	if(2000 + syear < 1970 || 2000 + syear > 2099)	
		return 1;
	
	for(t = 1970; t < 2000 + syear; t++)				//��������ݵ��������
	{
		if(Is_Leap_Year(t))
			seccount += 31622400;				//�����������
		else 
			seccount += 31536000;				//ƽ���������
	}
	
	smon -= 1;
	for(t = 0; t < smon; t++)					//��ǰ���·ݵ����������
	{
		seccount += (u32)mon_table[t] * 86400;	//�·����������
		if(Is_Leap_Year(2000 + syear) && t == 1)
			seccount += 86400;					//����2�·�����һ���������	   
	}
	seccount += (u32)(sday - 1) * 86400;		//��ǰ�����ڵ���������� 
	seccount += (u32)hour * 3600;					//Сʱ������
  seccount += (u32)min * 60;					//����������
	seccount += sec;							//�������Ӽ���ȥ

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);//ʹ��PWR��BKP����ʱ��  
	PWR_BackupAccessCmd(ENABLE);	//ʹ��RTC�ͺ󱸼Ĵ������� 
	if(flag == 0) {	RTC_SetCounter(seccount);	}	//����RTC��������ֵ
	
	RTC_WaitForLastTask();			//�ȴ����һ�ζ�RTC�Ĵ�����д�������  	
	return seccount;	    
}

/*flag : whether update local_date and local_time*/
/*flag == 0 used bacnet trendlog*/
void Get_Time_by_sec(u32 sec_time,UN_Time * rtc, uint8_t flag)
{
	static u16 daycnt = 0;
	u32 temp = 0;
	u16 temp1 = 0;
	
	
 	temp = sec_time / 86400;		//�õ�����(��������Ӧ��)
	if(flag == 0)
	{
		daycnt = 0;
	}
	if(daycnt != temp)				//����һ����
	{	  
		daycnt = temp;
		temp1 = 1970;				//��1970�꿪ʼ
		while(temp >= 365)
		{				 
			if(Is_Leap_Year(temp1))	//������
			{
				if(temp >= 366)
				{
					temp -= 366;	//�����������
				}
				else 
				{//??????????????????????
					// ��������һ�����
					//temp1++;
					break;
				}  
			}
			else
			{
				temp -= 365;		//ƽ��
			}			
			temp1++;  
		}   
		rtc->Clk.year = temp1 - 2000;	//�õ����
		rtc->Clk.day_of_year = temp + 1;  // get day of year, added by chelsea
		temp1 = 0;
		while(temp >= 28)			//������һ����
		{
			if(Is_Leap_Year(rtc->Clk.year) && temp1 == 1)	//�����ǲ�������/2�·�
			{
				if(temp >= 29)
					temp -=	29;		//�����������
				else
					break; 
			}
			else 
			{
				if(temp >= mon_table[temp1])
					temp -= mon_table[temp1];	//ƽ��
				else
					break;
			}
			temp1++;  
		}
		rtc->Clk.mon = temp1 + 1;	//�õ��·�
		rtc->Clk.day = temp + 1;  	//�õ����� 
	}
	temp = sec_time % 86400;     		//�õ�������   	   
	rtc->Clk.hour = temp / 3600;     	//Сʱ
	rtc->Clk.min = (temp % 3600) / 60; 	//����	
	rtc->Clk.sec = (temp % 3600) % 60; 	//����
	rtc->Clk.week = RTC_Get_Week(2000 + rtc->Clk.year, rtc->Clk.mon,rtc->Clk.day);	//��ȡ����   
	
	if(flag == 1)
	{
	Local_Date.year = rtc->Clk.year + 2000;
	Local_Date.month = rtc->Clk.mon;
	Local_Date.day = rtc->Clk.day;
	Local_Date.wday = rtc->Clk.week;
	
	Local_Time.hour = rtc->Clk.hour;
	Local_Time.min = rtc->Clk.min;
	Local_Time.sec = rtc->Clk.sec;
	}
}



//�õ���ǰ��ʱ��
//����ֵ:0,�ɹ�;����:�������.
u8 RTC_Get(void)
{
  Get_Time_by_sec(RTC_GetCounter(),&Rtc,1);
	return 0;
}

//������������ڼ�
//��������:���빫�����ڵõ�����(ֻ����1901-2099��)
//������������������� 
//����ֵ�����ں�																						 
u8 RTC_Get_Week(u16 year, u8 month, u8 day)
{	
	u16 temp2;
	u8 yearH, yearL;
	
	yearH = year / 100;
	yearL = year % 100;
	  
	if(yearH > 19)	// ���Ϊ21����,�������100
		yearL += 100;
	
	// ����������ֻ��1900��֮���  
	temp2 = yearL + yearL / 4;
	temp2 = temp2 % 7; 
	temp2 = temp2 + day + table_week[month - 1];
	
	if(yearL % 4 == 0 && month < 3)
		temp2--;
	
	return(temp2 % 7);
}			  
