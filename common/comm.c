 /*================================================================================
 * Module Name : comm.c
 * Purpose     : communicate with the slaver (C8051F023) with SPI bus
 * Author      : Chelsea
 * Date        : 2010/02/03
 * Notes       : 
 * Revision	   : 
 *	rev1.0
 *   
 *================================================================================
 */


#include "e2prom.h"
#include "comm.h"
#if ASIX_MINI
#include "projdefs.h"
#include "portable.h"
#include "errors.h"
#include "task.h"
#include "list.h"
#include "queue.h"
#include "spi.h"
#include "spiapi.h"
#include "stdio.h"
#endif
#include "define.h"

#include "main.h"


xTaskHandle xHandler_SPI;

#define SpiSTACK_SIZE		( ( unsigned portSHORT ) 256 )

#define MAX_LOSE_TOP 30

/**************************************************************************/
//3 commands between BOT and TOP
//G_TOP_CHIP_INFO  -- return information of TOP. Hardware rev and firmware rev and so on. It is not important.
//G_ALL -- return switch status (24 * 1 bytes)and input value( 32 * 2 bytes) and high speed counter( 6 * 4bytes)
//S_ALL -- send LED status to TOP. Communication LED( 2 bytes) + output led( 24 * 1byte) + input led( 32 * 1) + flag of high speed counter( 6 * 1)
/***************************************************************************/


U8_T	far flag_send_start_comm = 1;
U8_T	far flag_get_chip_info = 0;
U8_T data count_send_start_comm;
static U8_T far flag_lose_comm = 0;
static U8_T far count_lose_comm = 0;

extern U8_T flag_read_switch;


U16_T far Test[50];
U8_T re_send_led_in = 0;
U8_T re_send_led_out = 0;


bit flagLED_ether_tx = 0;
bit flagLED_ether_rx = 0;
bit flagLED_uart0_rx = 0;
bit flagLED_uart0_tx = 0;
bit flagLED_uart1_rx = 0; 
bit flagLED_uart1_tx = 0;
bit flagLED_uart2_rx = 0;
bit flagLED_uart2_tx = 0;
bit flagLED_usb_rx = 0;
bit flagLED_usb_tx = 0;




U8_T uart0_heartbeat = 0;
U8_T uart1_heartbeat = 0;
U8_T uart2_heartbeat = 0;
U8_T etr_heartbeat = 0;
U8_T usb_heartbeat = 0;	

U8_T flag_led_out_changed = 0;	  
U8_T flag_led_in_changed = 0;
U8_T flag_led_comm_changed = 0;
U8_T flag_high_spd_changed = 0;
U8_T CommLed[2];
U8_T far InputLed[32];  // high 4 bits - input type, low 4 bits - brightness
extern U8_T far input_type[32];
extern U8_T far input_type1[32];
U8_T OutputLed[24];
static U8_T OLD_COMM;
U8_T far high_spd_flag[HI_COMMON_CHANNEL];
U8_T far clear_high_spd[HI_COMMON_CHANNEL];
U16_T far count_clear_hsp[HI_COMMON_CHANNEL] = {0,0,0,0,0,0};
#if (ARM_MINI || ARM_TSTAT_WIFI)
U8_T far flag_count_in[HI_COMMON_CHANNEL];
#endif
U8_T xdata tmpbuf[150];

//U32_T far input_raw_temp[32];
//U8_T far raw_number[32];

U8_T far spi_index;

void initial_input_filter_data(void);
void Updata_Comm_Led(void);
U16_T crc16(U8_T *p, U8_T length);

#if (ARM_MINI || ASIX_MINI)


#define TEMPER_0_C   191*4
#define TEMPER_10_C   167*4
#define TEMPER_20_C   141*4
#define TEMPER_30_C   115*4
#define TEMPER_40_C   92*4


void vStartCommToTopTasks( unsigned char uxPriority)
{
//	U8_T base_hsp;
	U8_T i;
#if ARM_MINI
	//if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
	{	
		SPI1_Init(0);
	}
	
#endif
	
	memset(InputLed,0x35,32);
	memset(OutputLed,5,24);		
	
	memset(tmpbuf,0,150);
	memset(CommLed,0,2);
	memset(chip_info,0,12);
	OLD_COMM = 0;
	flag_send_start_comm = 1;
	flag_get_chip_info = 0;
	count_send_start_comm = 0;
 	flag_lose_comm = 0;
	count_lose_comm = 0;
	re_send_led_in = 0;
	re_send_led_out = 0;

	flagLED_ether_tx = 0;
	flagLED_ether_rx = 0;
	flagLED_uart0_rx = 0;
	flagLED_uart0_tx = 0;
	flagLED_uart1_rx = 0; 
	flagLED_uart1_tx = 0;
	flagLED_uart2_rx = 0;
	flagLED_uart2_tx = 0;
	flagLED_usb_rx = 0;
	flagLED_usb_tx = 0;		
	
	flag_led_out_changed = 0;	  
	flag_led_in_changed = 0;
	flag_led_comm_changed = 0;
	flag_high_spd_changed = 0;

	memset(high_spd_counter,0,4*HI_COMMON_CHANNEL);
	memset(high_spd_counter_tempbuf,0,4*HI_COMMON_CHANNEL);
//	if(Modbus.mini_type == MINI_BIG)
//	{
//		base_hsp = 26;
//	}
//	else if(Modbus.mini_type == MINI_SMALL)
//	{
//		base_hsp = 10;
//	}
		
	for(i = 0;i < HI_COMMON_CHANNEL;i++)
	{
		if((inputs[i].range == HI_spd_count) || (inputs[i].range == N0_2_32counts) 
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
			|| (inputs[i].range == RPM)
#endif
		)
		{
			high_spd_counter[i] = swap_double(inputs[i].value) / 1000;
			high_spd_counter_tempbuf[i] = 0;
		}
		count_clear_hsp[i] = 0;
		high_spd_en[i] = 0;
		high_spd_flag[i] = 0;
	}

//	memset(input_raw_temp,0,32*4);
//	memset(raw_number,0,32);


	if(Modbus.mini_type == MINI_TINY)
		OLD_COMM = 0;	
	else
	{
		initial_input_filter_data();
		Start_Comm_Top();
	}
	
	sTaskCreate( SPI_Roution, "SPITask", SpiSTACK_SIZE, NULL, uxPriority, &xHandler_SPI );
}



void Check_Pulse_Counter(void)
{
  char loop;	
	U8_T shift;
	shift = 1;
		
	if(chip_info[1] >= 42) 
		shift = 4;
	else 
		shift = 1;
	
	for(loop = 0;loop < HI_COMMON_CHANNEL;loop++)
	{
		if(high_spd_flag[loop] == 1) // start
		{ 
			if((input_raw_back[loop] != input_raw[loop]) && (input_raw[loop] > 600 * shift) 
				&& (input_raw_back[loop] < 400 * shift))  // from low to high
			{
				high_spd_counter_tempbuf[loop]++;
			}
		}
		if(high_spd_flag[loop] == 2) // clear
		{
			high_spd_counter_tempbuf[loop] = 0;
			inputs[loop].value = 0;
		}
		
		// backup input only when value is changed 
		if((input_raw_back[loop] > input_raw[loop] + 400 * shift)
			|| (input_raw[loop] > input_raw_back[loop] + 400 * shift))
		{
			input_raw_back[loop] = input_raw[loop];
		}
	}

	
	for(loop = 0;loop < HI_COMMON_CHANNEL;loop++)
	{ 
	// high_spd_flag 0: disable high speed counter  1:  start count 2: clear counter
		if(clear_high_spd[loop] == 1) // write 0 to clear it by T3000.
		{
			high_spd_flag[loop] = 2;  // clear
			count_clear_hsp[loop]++;
			if(count_clear_hsp[loop] > 20)
			{
				count_clear_hsp[loop] = 0;
				clear_high_spd[loop] = 0;
			}
		}
		else
		{			
			if((inputs[loop].range == HI_spd_count) || (inputs[loop].range == N0_2_32counts) 
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
				|| (inputs[loop].range == RPM)
#endif
			)
			{
				//high_spd_flag[HI_COMMON_CHANNEL - loop - 1] = high_spd_en[HI_COMMON_CHANNEL - loop - 1] + 1;
				high_spd_flag[loop] = 1; // start
			}
			else
			{
				high_spd_flag[loop] = 0; // stop
			}
		}
	}
}


// update led and input_type
void Update_Led(void)
{
	U8_T loop;
	
//	U8_T index,shift;
//	U32_T tempvalue;
	static U16_T pre_in[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	U8_T error_in; // error of input raw value	
	U8_T pre_status;
	U8_T max_in,max_out,max_digout;
	/*    check input led status */	
	U8_T shift;

	if(chip_info[1] >= 42) 
		shift = 4;
	else 
		shift = 1;

	if((Modbus.mini_type == MINI_BIG) || (Modbus.mini_type == MINI_BIG_ARM))
	{
		max_in = 32;
		max_out = 24;
		max_digout = 12;
	}
	else if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
	{
		max_in = 16;
		max_out = 10;
		max_digout = 6;
	}
#if ASIX_MINI
	else  if(Modbus.mini_type == MINI_TINY)
	{
		max_in = 11;
		max_out = 8;
		max_digout = 2;	
	}
#endif
	else  if((Modbus.mini_type == MINI_NEW_TINY) || (Modbus.mini_type == MINI_TINY_ARM))
	{
		max_in = 8;
		max_out = 14;
		max_digout = 6;
		if(outputs[6].digital_analog == 0)
			max_digout++;
		if(outputs[7].digital_analog == 0)
			max_digout++;	
	}
	else  if(Modbus.mini_type == MINI_TINY_11I)
	{
		max_in = 11;
		max_out = 11;
		max_digout = 6;
		
	}
	for(loop = 0;loop < max_in;loop++)
	{	
		pre_status = InputLed[loop];
	   
		if(input_raw[loop]  > pre_in[loop])
			error_in = input_raw[loop]  - pre_in[loop];
		else
			error_in = pre_in[loop] - input_raw[loop];					
		
		
		
		if(inputs[loop].range == not_used_input)
			InputLed[loop] = 0;	
		else
		{
			if(inputs[loop].auto_manual == 0) // auto
			{
				if(inputs[loop].digital_analog == 1) // analog 
				{
					if(inputs[loop].range <= PT1000_200_570DegF)	  // temperature
					{	//  10k termistor GREYSTONE
						if(input_raw[loop]  > TEMPER_0_C * shift) 	InputLed[loop] = 0;	   // 0 degree
						else  if(input_raw[loop]  > TEMPER_10_C * shift) 	InputLed[loop] = 1;	// 10 degree
						else  if(input_raw[loop]  > TEMPER_20_C * shift) 	InputLed[loop] = 2;	// 20 degree
						else  if(input_raw[loop]  > TEMPER_30_C * shift) 	InputLed[loop] = 3;	// 30 degree
						else  if(input_raw[loop]  > TEMPER_40_C * shift) 	InputLed[loop] = 4;	// 40 degree
						else
							InputLed[loop] = 5;	   // > 50 degree

						//InputLed high 4 bits - input type,
	//					InputLed[loop] |= 0x30;   // input type is 3
						
					}	
#if (ARM_MINI || ARM_TSTAT_WIFI)
					else if((inputs[loop].range == HI_spd_count) 
						|| (inputs[loop].range == N0_2_32counts) 
						|| (inputs[loop].range == RPM))
					{
						if(flag_count_in[loop] == 1)	{InputLed[loop] = 5; flag_count_in[loop] = 0;}
						else
						{InputLed[loop] = 0; }
					}	
#endif					
					else 	  // voltage or current
					{
						if(input_raw[loop]  < 50 * shift) 	InputLed[loop] = 0;
						else  if(input_raw[loop] < 200 * shift) 	InputLed[loop] = 1;
						else  if(input_raw[loop] < 400 * shift) 	InputLed[loop] = 2;
						else  if(input_raw[loop] < 600 * shift) 	InputLed[loop] = 3;
						else  if(input_raw[loop] < 800 * shift) 	InputLed[loop] = 4;
						else
							InputLed[loop] = 5;

					}

				}
				else if(inputs[loop].digital_analog == 0) // digtial
				{
					if(( inputs[loop].range >= ON_OFF  && inputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(inputs[loop].range >= custom_digital1 // customer digital unit
					&& inputs[loop].range <= custom_digital8
					&& digi_units[inputs[loop].range - custom_digital1].direct == 1))
					{// inverse
						if(inputs[loop].control == 1) InputLed[loop] = 0;	
						else
							InputLed[loop] = 5;	
					}
					else
					{
						if(inputs[loop].control == 1) InputLed[loop] = 5;	
						else
							InputLed[loop] = 0;	
					}
				}
			}
			else // manual
			{
				if(inputs[loop].digital_analog == 0) // digtial
				{
					if(( inputs[loop].range >= ON_OFF  && inputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(inputs[loop].range >= custom_digital1 // customer digital unit
					&& inputs[loop].range <= custom_digital8
					&& digi_units[inputs[loop].range - custom_digital1].direct == 1))
					{ // inverse
						if(inputs[loop].control == 1) InputLed[loop] = 0;	
						else
							InputLed[loop] = 5;	
					}
					else
					{
						if(inputs[loop].control == 1) InputLed[loop] = 5;	
						else
							InputLed[loop] = 0;				
					}
						
				}
				else   // analog
				{
					U32_T tempvalue;
					tempvalue = swap_double(inputs[loop].value) / 1000;
					if(inputs[loop].range <= PT1000_200_570DegF)	  // temperature
					{	//  10k termistor GREYSTONE
						if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
						else  if(tempvalue < 10) 	InputLed[loop] = 1;	// 10 degree
						else  if(tempvalue < 20) 	InputLed[loop] = 2;	// 20 degree
						else  if(tempvalue < 30) 	InputLed[loop] = 3;	// 30 degree
						else  if(tempvalue < 40) 	InputLed[loop] = 4;	// 40 degree
						else
							InputLed[loop] = 5;	   // > 50 degree						
					}		
					else 	  // voltage or current
					{						
						//InputLed high 4 bits - input type,
						if(inputs[loop].range == V0_5 || inputs[loop].range == P0_100_0_5V)	
						{						
							if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 1) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 2) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 3) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 4) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree	
						}
						if(inputs[loop].range == I0_20ma)	
						{
							if(tempvalue <= 4) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 7) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 10) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 14) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 18) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree	
						}
						if(inputs[loop].range == V0_10_IN || inputs[loop].range == P0_100_0_10V)	
						{
							if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 2) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 4) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 6) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 8) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree	
						}

					}
				}
			}
		}	
		
		if(pre_status != InputLed[loop] && error_in > 50 * shift)
		{  //  error is larger than 20, led of input is changed
			flag_led_in_changed = 1;   
			re_send_led_in = 0;
		}
		pre_in[loop] = input_raw[loop];

		//if(Setting_Info.reg.pro_info.firmware_rev >= 14)
		{
			InputLed[loop] &= 0x0f;		
			if(input_type[loop] >= 1)
				InputLed[loop] |= ((input_type[loop] - 1) << 4);			
			else
				InputLed[loop] |= (input_type[loop] << 4);
		}
	} 
	
	if(flag_read_switch == 1) 
	{
		/*    check output led status */	
		for(loop = 0;loop < max_out;loop++)
		{	
	//		OutputLed[loop] = loop / 4;
			pre_status = OutputLed[loop];
			if(outputs[loop].switch_status == SW_AUTO)
			{
				if(outputs[loop].range == 0)
				{
					OutputLed[loop] = 0;
				}
				else
				{
					if(loop < max_digout)	  // digital
					{
						if(swap_double(outputs[loop].value) == 0) OutputLed[loop] = 0;
						else
							OutputLed[loop] = 5;
					}
					else
					{// hardwar AO
						if(outputs[loop].digital_analog == 1)
						{
							if(output_raw[loop] >= 0 && output_raw[loop] < 50 )	   			
								OutputLed[loop] = 0;
							else if(output_raw[loop] >= 50 && output_raw[loop] < 200 )		
								OutputLed[loop] = 1;
							else if(output_raw[loop] >= 200 && output_raw[loop] < 400 )		
								OutputLed[loop] = 2;
							else if(output_raw[loop] >= 400 && output_raw[loop] < 600 )		
								OutputLed[loop] = 3;
							else if(output_raw[loop] >= 600 && output_raw[loop] < 800 )		
								OutputLed[loop] = 4;
							else if(output_raw[loop] >= 800 && output_raw[loop] < 1023 )		
								OutputLed[loop] = 5;
						}
						else
						{  // AO is used for DO
							if(( outputs[loop].range >= ON_OFF  && outputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
							||(outputs[loop].range >= custom_digital1 // customer digital unit
							&& outputs[loop].range <= custom_digital8
							&& digi_units[outputs[loop].range - custom_digital1].direct == 1))
							{ // inverse
								if(outputs[loop].control == 1) OutputLed[loop] = 0;	
								else
									OutputLed[loop] = 5;	
							}
							else
							{
								if(outputs[loop].control == 1) OutputLed[loop] = 5;	
								else
									OutputLed[loop] = 0;				
							}
						}
					}
				}
			}
			else if(outputs[loop].switch_status == SW_OFF)		OutputLed[loop] = 0;
			else if(outputs[loop].switch_status == SW_HAND)	
			{
				if(outputs[loop].range != 0)
					OutputLed[loop] = 5;
			}

			if(pre_status != OutputLed[loop])
			{
				flag_led_out_changed = 1;  
				re_send_led_out = 0;
			}
		}
	}
	else
	{ // turn off all outputs before switch status are ready
		for(loop = 0;loop < max_out;loop++)
			OutputLed[loop] = 0;
	}

	/*    check communication led status */

	Updata_Comm_Led();
}


void Updata_Comm_Led(void)
{
	U8_T temp1 = 0;
	U8_T temp2 = 0;
	U8_T pre_status1 = CommLed[0];
	U8_T pre_status2 = CommLed[1];
//	U8_T i;
	
	temp1 = 0;
	temp2 = 0;
	pre_status1 = CommLed[0];
	pre_status2 = CommLed[1];
	if((Modbus.mini_type == MINI_BIG) || (Modbus.mini_type == MINI_BIG_ARM))
	{
		if(Modbus.hardRev >= 21)  // swap UART0 and UART1 
		{
			if(flagLED_uart2_rx)	{ temp1 |= 0x02;	 	flagLED_uart2_rx = 0;}
			if(flagLED_uart2_tx)	{	temp1 |= 0x01;		flagLED_uart2_tx = 0;}	
			if(flagLED_uart0_rx)	{	temp1 |= 0x08;		flagLED_uart0_rx = 0;}
			if(flagLED_uart0_tx)	{	temp1 |= 0x04;		flagLED_uart0_tx = 0;}	
		}
		else
		{
			if(flagLED_uart0_rx)	{ temp1 |= 0x02;	 	flagLED_uart0_rx = 0;}
			if(flagLED_uart0_tx)	{	temp1 |= 0x01;		flagLED_uart0_tx = 0;}	
			if(flagLED_uart2_rx)	{	temp1 |= 0x08;		flagLED_uart2_rx = 0;}
			if(flagLED_uart2_tx)	{	temp1 |= 0x04;		flagLED_uart2_tx = 0;}
		}
		
		if(flagLED_ether_rx)	{ temp1 |= 0x20;		flagLED_ether_rx = 0;}
		if(flagLED_ether_tx)	{	temp1 |= 0x10;		flagLED_ether_tx = 0;}
		
		
		
//		if(Modbus.com_config[1] != MODBUS_MASTER)
//		{
//			if(flagLED_usb_rx)		{	temp1 |= 0x40;	 	flagLED_usb_rx = 0;}
//			if(flagLED_usb_tx)		{	temp1 |= 0x80;		flagLED_usb_tx = 0;} 
//		}
		
		if(Modbus.com_config[1] == MODBUS_MASTER)
		{
			if(flagLED_uart1_rx)	{	temp1 |= 0x80;	 flagLED_uart1_rx = 0;}
			if(flagLED_uart1_tx)	{	temp1 |= 0x40;	 flagLED_uart1_tx = 0;}
		}
		else
		{
			if(flagLED_uart1_rx)	{	temp2 |= 0x02;	 	flagLED_uart1_rx = 0;}
			if(flagLED_uart1_tx)	{	temp2 |= 0x01;		flagLED_uart1_tx = 0;} 
		}
	}
	else  if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
	{				
		if(flagLED_uart2_rx)	{ temp1 |= 0x02;	 	flagLED_uart2_rx = 0; }
		if(flagLED_uart2_tx)	{	temp1 |= 0x01;		flagLED_uart2_tx = 0; }	

		if(flagLED_ether_rx)	{	temp1 |= 0x08;		flagLED_ether_rx = 0; }			
		if(flagLED_ether_tx)	{	temp1 |= 0x04;		flagLED_ether_tx = 0;}		
		
		if(flagLED_uart0_rx)	{ temp1 |= 0x20;		flagLED_uart0_rx = 0;}
		if(flagLED_uart0_tx)	{	temp1 |= 0x10;		flagLED_uart0_tx = 0;}
//		if(flagLED_usb_rx)		{	temp1 |= 0x40;	 	flagLED_usb_rx = 0;}
//		if(flagLED_usb_tx)		{	temp1 |= 0x80;		flagLED_usb_tx = 0;} 
		
//		if(flagLED_uart1_tx)	{	temp2 |= 0x01;		flagLED_uart1_tx = 0;} 
//		if(flagLED_uart1_rx)	{	temp2 |= 0x02;	 	flagLED_uart1_rx = 0;}

		if(flagLED_uart1_tx)	{	temp1 |= 0x40;		flagLED_uart1_tx = 0;} 
		if(flagLED_uart1_rx)	{	temp1 |= 0x80;	 	flagLED_uart1_rx = 0;}
	}
	else  if(Modbus.mini_type == MINI_TINY)
	{
		// TBD: 
		if(flagLED_uart2_rx)	{ temp1 |= 0x10;	 	flagLED_uart2_rx = 0;}
		if(flagLED_uart2_tx)	{	temp1 |= 0x20;		flagLED_uart2_tx = 0;}	
		if(flagLED_ether_rx)	{	temp1 |= 0x04;		flagLED_ether_rx = 0;}
		if(flagLED_ether_tx)	{	temp1 |= 0x08;		flagLED_ether_tx = 0;}
		if(flagLED_uart0_rx)	{ temp1 |= 0x01;		flagLED_uart0_rx = 0;}
		if(flagLED_uart0_tx)	{	temp1 |= 0x02;		flagLED_uart0_tx = 0;}
	}
	else  if((Modbus.mini_type == MINI_NEW_TINY) || (Modbus.mini_type == MINI_TINY_ARM) || (Modbus.mini_type == MINI_TINY_11I))
	{
		// TBD: 
//		if(flagLED_uart2_rx)	{ temp1 |= 0x10;	 	flagLED_uart2_rx = 0;}
//		if(flagLED_uart2_tx)	{	temp1 |= 0x20;		flagLED_uart2_tx = 0;}	
//		if(flagLED_ether_rx)	{	temp1 |= 0x04;		flagLED_ether_rx = 0;}
//		if(flagLED_ether_tx)	{	temp1 |= 0x08;		flagLED_ether_tx = 0;}
//		if(flagLED_uart0_rx)	{ temp1 |= 0x01;		flagLED_uart0_rx = 0;}
//		if(flagLED_uart0_tx)	{	temp1 |= 0x02;		flagLED_uart0_tx = 0;}
	}
	CommLed[0] = temp1;
	if(pre_status1 != CommLed[0])
		flag_led_comm_changed = 1;

	CommLed[1] = temp2;
	if(pre_status2 != CommLed[1])
		flag_led_comm_changed = 1;

}

void SPI_Send(U8_T cmd,U8_T* buf,U8_T len)
{	 
	U8_T i;
	u16 crc;
#if ASIX_MINI
	if(cSemaphoreTake(sem_SPI, 0) == pdFALSE)
		return ;
#endif	
	
#if ARM_MINI
	if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
	{
		if(cSemaphoreTake(sem_SPI, 5) == pdFALSE)
			return ;
	}
#endif
  SPI_ByteWrite(cmd);
	
	for(i = 0; i < len; i++) 
	{		
		SPI_ByteWrite(buf[i]);
	}
	
	// add crc
	if(cmd == S_ALL_NEW)
	{
		crc = crc16(buf,len);
		SPI_ByteWrite(crc / 256);
		SPI_ByteWrite(crc % 256);
	}
	else
	{
		SPI_ByteWrite(0x55);
		SPI_ByteWrite(0xaa);
	}

#if ASIX_MINI
	cSemaphoreGive(sem_SPI);
#endif
	
#if ARM_MINI
	if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
		cSemaphoreGive(sem_SPI);
#endif
}


void SPI_Get(U8_T cmd,U8_T len)
{
	U8_T i;
	U8_T rec_len = 0;
	U8_T error = 1;
	U8_T count_error = 0;
	U8_T crc[2];
	U16_T temp;

#if ASIX_MINI	
	/*send the first byte, command type */
	if(cSemaphoreTake(sem_SPI, 5) == pdFALSE)
		return ;
#endif
	
#if ARM_MINI
	if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
	{
		if(cSemaphoreTake(sem_SPI, 5) == pdFALSE)
			return ;
	}
#endif
	SPI_ByteWrite(cmd);		
	
	SPI_ByteWrite(0xff);

#if ASIX_MINI	
	if((Modbus.mini_type == MINI_TINY) && (Modbus.hardRev < STM_TINY_REV))
	{
		rec_len = len + 3;
	}
	else
#endif
		rec_len = len + 2;

	
	for(i = 0; i < rec_len; i++) 
	{	
#if ASIX_MINI
		SPI_ByteWrite(0xff);		
		SPI_GetData(&tmpbuf[i]);
#endif
		
#if ARM_MINI	
		tmpbuf[i] = SPI1_ReadWriteByte(0xff);
		//if(i == rec_len - 1)  delay_ms(20);
#endif
		
		if(tmpbuf[i] != 0xff)  
			error = 0;
	}	

//	SPI1_CS_SET();	
#if ASIX_MINI // old top board
	if((Modbus.mini_type == MINI_TINY) && (Modbus.hardRev < STM_TINY_REV))
	{
		if((crc[0]!= 0x55) ||(crc[1] != 0xd5))	 {error = 1; } 	
		else
			error = 0;
	}
	else
	{
		if((crc[0]!= 0xaa) ||(crc[1] != 0x55))	 error = 1; 
		else
			error = 0;
	}
#endif
	
#if (ARM_MINI || ASIX_MINI)
	if(cmd == G_ALL_NEW)
	{
		u16 crc_check;		
		crc_check = crc16(tmpbuf, i - 2); // crc16
		
		if((HIGH_BYTE(crc_check) == tmpbuf[i - 2]) && (LOW_BYTE(crc_check) == tmpbuf[i - 1]))
		{
			error = 0;
		}
		else
		{
			error = 1;
		}
	}
	else
	{
		if((0x55 == tmpbuf[i - 2]) && (0xaa == tmpbuf[i - 1]))
		{
			error = 0;
		}
		else
		{
			error = 1;
		}
	}

#endif
	
	if((tmpbuf[0] == 0x55) && (tmpbuf[1] == 0x55) && (tmpbuf[2] == 0x55) && (tmpbuf[3] == 0x55) && (tmpbuf[4] == 0x55))
	{	// top board dont not get mini type
		Start_Comm_Top();
		error = 1;
	}

	
	if(error == 0) // no error
	{
		Test[1]++;
		flag_lose_comm = 0;
		count_lose_comm = 0;
		count_error = 0;
#if ASIX_MINI // old top board, old commucation protocal
		if(cmd == G_SWTICH_STATUS)
		{				
		  for(i = 0;i < len;i++)
			{// ONLY FOR OLD BOARD
				outputs[i].switch_status = SW_AUTO;//tmpbuf[i];
				flag_read_switch = 1;
			}
		}
		else if(cmd == G_INPUT_VALUE)
		{
		  for(i = 0;i < len / 2;i++)
			{				   
				input_raw[i] = (U16_T)(tmpbuf[i * 2 + 1] + tmpbuf[i * 2] * 256); //Filter(i,(U16_T)(tmpbuf[i * 2 + 1] + tmpbuf[i * 2] * 256));
				// add filter 
				//input_raw[i] = filter_input_raw(i,(U16_T)(tmpbuf[i * 2 + 1] + tmpbuf[i * 2] * 256));
			}
		} 
		else
#endif			
		if(cmd == G_TOP_CHIP_INFO)
		{
			if(Modbus.mini_type == MINI_TINY)
			{
				for(i = 0;i < 4;i++)
				{
					chip_info[i] = tmpbuf[i];
					Setting_Info.reg.pro_info.firmware_rev = chip_info[1];  
					Setting_Info.reg.pro_info.hardware_rev = chip_info[2];
					
					Setting_Info.reg.specila_flag &= 0xfe;
				}	
				flag_get_chip_info = 1;
				OLD_COMM = 0;				
			}
			else
			{
				for(i = 0;i < len / 2;i++)
				{
					if(i < 6)
					{
						chip_info[i] = (U16_T)(tmpbuf[i * 2 + 1] + tmpbuf[i * 2] * 256);						
					}
					Setting_Info.reg.pro_info.firmware_rev = chip_info[1];
					Setting_Info.reg.pro_info.hardware_rev = chip_info[0];
				}
				
				if(chip_info[1] >= 42 && chip_info[2] == 1) // rev42 add PT sensor
					Setting_Info.reg.specila_flag |= 0x01;
				else
					Setting_Info.reg.specila_flag &= 0xfe;					
				
				
				if(Setting_Info.reg.pro_info.firmware_rev > 0
					&& Setting_Info.reg.pro_info.hardware_rev > 0)
				{
					flag_get_chip_info = 1;
				}
				else
				{
					// reboot top board
					Test[40] = 22;
				}
				
				
#if (ARM_MINI || ASIX_ARM)
				// check whether hardware information is valid
				if( (chip_info[4] != 0) || (chip_info[5] != 0))  // SM5964 rev is 0, read wrong information
				{
						flag_get_chip_info = 0;
				}
#endif
				
				if(Setting_Info.reg.pro_info.firmware_rev >= 9)  // new comm
				{					
					OLD_COMM = 0;
				}
				else
				{	
					OLD_COMM = 1; 
				}
				
			}
		}
		else if((cmd == G_ALL) || (cmd == G_ALL_NEW))
		{
#if 	ASIX_MINI 
			if(Modbus.mini_type == MINI_TINY)  // tiny
			{
				for(i = 0;i < 8;i++)
				{
					if(tmpbuf[i] != outputs[i].switch_status)
					{
						outputs[i].switch_status = tmpbuf[i];
						check_output_priority_HOA(i);
					}
					flag_read_switch = 1;
				}
			
				if(Setting_Info.reg.pro_info.firmware_rev >= 30) // ARM board
				{
					for(i = 0;i < 22 / 2;i++)
					{		
						input_raw[i] = (U32_T)((tmpbuf[i * 2 + 1 + 8] + tmpbuf[i * 2 + 8] * 256)) * 1023 / Modbus.vcc_adc;

					}
					for(i = 0;i < 24 / 4;i++)
					{		
						char start;
//						if(Modbus.mini_type == MINI_BIG)	start = 26;
//						else if(Modbus.mini_type == MINI_SMALL) start = 10;		
//						else 
//							if(Modbus.mini_type == MINI_TINY) 
						start = 5;		
						high_spd_counter_tempbuf[start + i] = swap_double( tmpbuf[i * 4 + 33] | (U16_T)tmpbuf[i * 4 + 32] << 8 | (U32_T)tmpbuf[i * 4 + 31] << 16 |  (U32_T)tmpbuf[i * 4 + 30] << 24);
						
					}	
					for(i = 0;i < 8 / 2;i++)
					{								
						AO_feedback[i] = (U32_T)(tmpbuf[i * 2 + 55] + tmpbuf[i * 2 + 54] * 256);
								//						13;  // 13 is  0.04v ������;
						if(AO_feedback[i] < 450)
							AO_feedback[i] += 1000 / AO_feedback[i];
						else
							AO_feedback[i] -= AO_feedback[i] / 50;						
						
					}	
				}
			}
			else  // big and small panel
#endif
			{			
				if((Modbus.mini_type == MINI_BIG) || (Modbus.mini_type == MINI_BIG_ARM))
				{
					U8_T packet_error;
					packet_error = 0;
					for(i = 0;i < 24;i++)
					{
						if(tmpbuf[i] > 2) 			packet_error = 1;
					}
					
					if(packet_error == 0)
					{
						flag_read_switch = 1;
						for(i = 0;i < 24;i++)
						{
							if((tmpbuf[i] != outputs[i].switch_status) &&
									(tmpbuf[i] <= 2))						
							{
								outputs[i].switch_status = tmpbuf[i];	
								check_output_priority_HOA(i);	
							}							
						}
						for(i = 0;i < 64 / 2;i++)	  // 88 == 24+64
						{							
							temp = Filter(i,(U16_T)(tmpbuf[i * 2 + 1 + 24] + tmpbuf[i * 2 + 24] * 256));
							if(temp != 0xffff)
							{// rev42 of top is 12bit, older rev is 10bit
									//if(chip_info[1] >= 42)
										// new input moudle with PT sensor
										input_raw[i] = (U32_T)temp * 1023 / Modbus.vcc_adc;
	//								else
	//									// for old input moudle
	//									input_raw[i] = (U32_T)temp * 255 / Modbus.vcc_adc;	
							}

						}
					}
				}
				if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
				{
					// check packet is correct, only for LB_ARM
					U8_T packet_error;
					packet_error = 0;
					for(i = 0;i < 10;i++)
					{
						if(tmpbuf[i] > 2) 			packet_error = 1;
					}
					
					if(packet_error == 0)
					{
						flag_read_switch = 1;					

						for(i = 0;i < 10;i++)
						{
							if((tmpbuf[i] != outputs[i].switch_status) &&
								(tmpbuf[i] <= 2))
							{	
								outputs[i].switch_status = tmpbuf[i];		
								check_output_priority_HOA(i);			
							}
						}
						
						for(i = 0;i < 32 / 2;i++)	  // 88 == 24+64
						{	
							temp = Filter(i,(U16_T)(tmpbuf[i * 2 + 1 + 24] + tmpbuf[i * 2 + 24] * 256));
							if(temp != 0xffff)
							{// rev42 of top is 12bit, older rev is 10bit
								//if(chip_info[1] >= 42)
									// new input moudle with PT sensor
									input_raw[i] = (U32_T)temp * 1023 / Modbus.vcc_adc;
//								else
//									// for old input moudle
//									input_raw[i] = (U32_T)temp * 255 / Modbus.vcc_adc;	
								
							}
						}
					}
				}
				if(Setting_Info.reg.pro_info.firmware_rev >= 30) // ARM board
				{
					
					for(i = 0;i < 24 / 4;i++)
					{		
						char start;
						if((Modbus.mini_type == MINI_BIG) || (Modbus.mini_type == MINI_BIG_ARM))	start = 26;
						else if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM)) start = 10;		
						//else if(Modbus.mini_type == MINI_TINY) start = 5;	
						//if(inputs[start + i].range == HI_spd_count)
						//high_spd_counter_tempbuf[start + i] = swap_double( tmpbuf[i * 4 + 91] | (U16_T)tmpbuf[i * 4 + 90] << 8 | (U32_T)tmpbuf[i * 4 + 89] << 16 |  (U32_T)tmpbuf[i * 4 + 88] << 24);
						high_spd_counter_tempbuf[start + i] = swap_double( tmpbuf[i * 4 + 88] | (U16_T)tmpbuf[i * 4 + 89] << 8 | (U32_T)tmpbuf[i * 4 + 90] << 16 |  (U32_T)tmpbuf[i * 4 + 91] << 24);
					}
				}
			}
		}
	}
	else
	{
		Test[0]++;

		flag_lose_comm = 1;
		count_lose_comm++;
	}

#if ASIX_MINI 
	cSemaphoreGive(sem_SPI);
#endif

	
#if ARM_MINI
	if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
		cSemaphoreGive(sem_SPI);
#endif


}

#if (ARM_MINI || ASIX_MINI)

void Check_whether_lose_comm(void)
{	

	Test[2] = count_lose_comm;
	if(flag_lose_comm)	
	{		
		if(count_lose_comm > MAX_LOSE_TOP)
		{	
	// generate a alarm
			generate_common_alarm(ALARM_LOST_TOP);
#if ASIX_MINI 			
			if((Modbus.mini_type == MINI_BIG) && (Modbus.hardRev <= 22))
			{
				RESET_TOP = 0;  // RESET c8051f023 
				DELAY_Ms(100);
				RESET_TOP = 1; 	
				flag_send_start_comm = 1;
				count_send_start_comm = 0;
			}
			if((Modbus.mini_type == MINI_SMALL) && (Modbus.hardRev <= 6))
			{

				RESET_TOP = 0;  // RESET c8051f023 
				DELAY_Ms(100);
				RESET_TOP = 1; 	
				flag_send_start_comm = 1;
				count_send_start_comm = 0;

			}
			else if(Modbus.mini_type == MINI_TINY)
			{
				if(Modbus.hardRev >= STM_TINY_REV)
				{					
					RESET_TOP = 0;  // RESET stm32
					DELAY_Ms(100);
					RESET_TOP = 1; 
				}
				else
				{  // old version, top board is sm5r16
				
					RESET_TOP = 1;  // RESET sm5r16 
					DELAY_Ms(100);
					RESET_TOP = 0;
					
				}
			}
#endif
			
#if ARM_MINI   // BB && LB
			if((Modbus.mini_type == MINI_BIG_ARM) || (Modbus.mini_type == MINI_SMALL_ARM))
			{
				RESET_TOP = 0;  // RESET c8051f023 
				DELAY_Ms(100);
				RESET_TOP = 1; 	
				flag_send_start_comm = 1;
				count_send_start_comm = 0;
				Test[3]++;
			}
			
#endif
			count_lose_comm = 0;
			flag_lose_comm = 0;
	
		}
	}
}
#endif	


void SPI_Roution(void)
{
	char i;
	portTickType xDelayPeriod = ( portTickType ) 25 / portTICK_RATE_MS;
	U8_T far send_all[100];
	task_test.enable[8] = 1;
	spi_index = 0;
	
	for (;;)
	{
		task_test.count[8]++;
		if(task_test.count[8] % 40 == 0)
			Check_Pulse_Counter();
		Update_Led();
		current_task = 8;
		Check_whether_lose_comm();
		

		// test wrong spi command
		if(Test[40] == 55)
		{
			SPI_ByteWrite(0x55);	
			vTaskDelay(75 / portTICK_RATE_MS);
		}
		else if(Test[40] == 11)
		{ // top reset bot
			SPI_ByteWrite(0x11);	
			vTaskDelay(500 / portTICK_RATE_MS);
		}
		else if(Test[40] == 22)
		{
			RESET_TOP = 0;  // RESET top
			DELAY_Ms(100);
			RESET_TOP = 1; 	
//			SPI_ByteWrite(0x55);
			Test[40] = 0;
		}
		else
		{
#if ASIX_MINI
		if(Modbus.mini_type == MINI_TINY)
		{			
			if(Setting_Info.reg.pro_info.firmware_rev >= 30)
				vTaskDelay(50 / portTICK_RATE_MS);
			else
				vTaskDelay(175 / portTICK_RATE_MS);
			
			
			if(flag_get_chip_info == 0) // get information first
			{	
				SPI_Get(G_TOP_CHIP_INFO,12);
			}
			else
			{
				if(spi_index == 0)
				{	
					spi_index = 1;
					//memcpy(send_all,CommLed,2);	
					memcpy(&send_all[0],OutputLed,8);
					
					if(Setting_Info.reg.pro_info.firmware_rev >= 30)  // ARM board
					{
						memcpy(&send_all[8],InputLed,11);	
						
					}
					else
					{
						send_all[8] = InputLed[0] & 0x0f;
						send_all[9] = InputLed[1] & 0x0f;
						send_all[10] = InputLed[2] & 0x0f;
						send_all[11] = InputLed[3] & 0x0f;
						send_all[12] = InputLed[4] & 0x0f;
						send_all[13] = InputLed[5] & 0x0f;
						send_all[14] = InputLed[6] & 0x0f;
						send_all[15] = InputLed[7] & 0x0f;
						send_all[16] = InputLed[8] & 0x0f;
						send_all[17] = InputLed[9] & 0x0f;
						send_all[18] = InputLed[10] & 0x0f;
					}
					
					send_all[23] = (CommLed[0] & 0x04) ? 5 : 0;
					send_all[24] = (CommLed[0] & 0x08) ? 5 : 0;
					send_all[19] = (CommLed[0] & 0x01) ? 5 : 0;
					send_all[20] = (CommLed[0] & 0x02) ? 5 : 0;
					send_all[21] = (CommLed[0] & 0x10) ? 5 : 0;
					send_all[22] = (CommLed[0] & 0x20) ? 5 : 0;
					
					if(Setting_Info.reg.pro_info.firmware_rev >= 30) // ARM board
					{// new hardware have high speed counter
						char i;
						
						memcpy(&send_all[25],&high_spd_flag[5],6);  // tiny have 6 HSP ,start pos is 5
#if ASIX_MINI						
						send_all[31] = relay_value.byte[1];   // new tiny control relays by ARM chip
						
						send_all[32] = relay_value.byte[0];
#endif
						if(Setting_Info.reg.pro_info.firmware_rev >= 45)
						{
							SPI_Send(S_ALL_NEW,send_all,33); // add CRC
						}
						else	
							SPI_Send(S_ALL,send_all,33);
					}
					else  // old version, do not have high speed counter					
						SPI_Send(S_ALL,send_all,25);
				}			
				else if(spi_index == 1)
				{	
					if(Setting_Info.reg.pro_info.firmware_rev >= 33) // ARM board  8 switch_status + 11 (* 2) input_value + 6 (* 4) highspeedcouner + 4 (*2) AO feedback
					{
						SPI_Get(G_ALL,62); 
					}
					else if(Setting_Info.reg.pro_info.firmware_rev >= 30) // ARM board  8 switch_status + 11 (* 2) input_value + 6 (* 4) highspeedcouner
					{
						SPI_Get(G_ALL,54);
					}
					else // old top board   8 switch_status  + 8 feedback
					{
						SPI_Get(G_ALL,16);
					}
					spi_index = 0;
				}
			}			
		}
		else
#endif
		{	

			vTaskDelay(50 / portTICK_RATE_MS);

			if(flag_send_start_comm)
			{
				if(count_send_start_comm < 30)
				{	
					count_send_start_comm++; 	
				}
				else
				{	
					
					Start_Comm_Top();
					flag_send_start_comm = 0; 
				}
			}
			else
			{	
#if ASIX_MINI		 // for very old hardware, top board is c8051		
				if(OLD_COMM == 1)
				{
					if(flag_get_chip_info == 0)
					{
						SPI_Get(G_TOP_CHIP_INFO,12);
					}
					else if(flag_led_out_changed)	
					{
						if(re_send_led_out < 10)
						{	
							SPI_Send(S_OUTPUT_LED,OutputLed,24);
							re_send_led_out++;
						}
						else
						{
							re_send_led_out = 0;
							flag_led_out_changed = 0;
						}
					}
					else if(flag_led_in_changed)	
					{	
						if(re_send_led_in < 10)
						{	
							SPI_Send(S_INPUT_LED,InputLed,32);
							re_send_led_in++;
						}
						else
						{
							re_send_led_in = 0;
							flag_led_in_changed = 0;
						}
					}
					else 
					{	
						if(spi_index == 0 || spi_index == 2 || spi_index == 4 || spi_index == 6)	 
							//SPI_Get(G_TOP_CHIP_INFO,12);//
							SPI_Send(S_COMM_LED,CommLed,2);
						else if(spi_index == 1)	 SPI_Send(S_OUTPUT_LED,OutputLed,24);
						else if(spi_index == 3)	 {
							
							SPI_Send(S_INPUT_LED,InputLed,32);
						}
						else 
						if(spi_index == 5)	 
						{			
							SPI_Get(G_SWTICH_STATUS,24);
						}
						else if(spi_index == 7)	 
						{	
							SPI_Get(G_INPUT_VALUE,64);				
						}
			//			else if((index == 6) && high_spd_status != 0)	 
			//				;//SPI_Get(G_SPEED_COUNTER,high_spd_counter,24);
			//	
						if(spi_index < 7)	
							spi_index++;
						else 
							spi_index = 0; 
					
					}
				}
				else
#endif
				{	
					if(flag_get_chip_info == 0)
					{	
						SPI_Get(G_TOP_CHIP_INFO,12);
					}
					else 
					{	
						if(spi_index == 0)
						{
							memcpy(send_all,CommLed,2);
							memcpy(&send_all[2],OutputLed,24);
#if ASIX_MINI  // old communcation protocal, old top board
							if(Setting_Info.reg.pro_info.firmware_rev <= 13)  // old top board
							{
								char i;
								for(i = 0;i < 32;i++)
									InputLed[i] &= 0x0f;
							}
#endif
							memcpy(&send_all[26],InputLed,32);

							
							if(Setting_Info.reg.pro_info.firmware_rev >= 10)  // C8051, new protocal
							{	
								if((Modbus.mini_type == MINI_BIG) || (Modbus.mini_type == MINI_BIG_ARM))
									memcpy(&send_all[58],&high_spd_flag[26],6);	 // HI_COMMON_CHANNEL 6
								else if((Modbus.mini_type == MINI_SMALL) || (Modbus.mini_type == MINI_SMALL_ARM))
									memcpy(&send_all[58],&high_spd_flag[10],6);	 // HI_COMMON_CHANNEL 6
								if(Setting_Info.reg.pro_info.firmware_rev >= 45)
								{
									SPI_Send(S_ALL_NEW,send_all,64); // add CRC
								}
								else
									SPI_Send(S_ALL,send_all,64);	
							}
#if ASIX_MINI  // old communcation protocal, old top board
							else if(Setting_Info.reg.pro_info.firmware_rev == 9)  // C8051, old protocal
							{
								SPI_Send(S_ALL,send_all,58);
							}
#endif							
							spi_index = 1;
							if(flag_led_comm_changed)
								flag_led_comm_changed = 0;
						}
						else 
						{	
#if ASIX_MINI 														
 // old communcation protocal, old top board
							if(Setting_Info.reg.pro_info.firmware_rev == 9)
							{
								SPI_Get(G_ALL,88);						
							}
							else
#endif
							if(Setting_Info.reg.pro_info.firmware_rev >= 45)
							{
								SPI_Get(G_ALL_NEW,112); // add CRC
							}
							else
							{
								SPI_Get(G_ALL,112);  // 88 + 24
							}

							spi_index = 0;
						}
					}
	
				}
	
			}

		}
	}
	}
}




void Start_Comm_Top(void)
{
	SPI_ByteWrite(C_MINITYPE);
	SPI_ByteWrite(Modbus.mini_type);
} 




#endif


