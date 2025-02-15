/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: tapdev.c,v 1.8 2006/06/07 08:39:58 adam Exp $
 */	    
#include "tapdev.h"
#include "uip.h"
#include "enc28j60.h"
#include <stdio.h>
#include "tcpip.h"
#include "define.h"

//#define DHCP_ENABLE

//用于固定IP地址开关打开后的IP设置，本例程没有用这个
#define UIP_DRIPADDR0   192
#define UIP_DRIPADDR1   168
#define UIP_DRIPADDR2   0
#define UIP_DRIPADDR3   111

void uip_polling(void);
//MAC地址,必须唯一
//const u8 mymac[6]={0x04, 0x02, 0x35, 0x0F, 0x00, 0x01};	//MAC地址
extern uint32 multicast_addr;																				  
//配置网卡硬件，并设置MAC地址 
//返回值：0，正常；1，失败；
u8 tapdev_init(void)
{  
	u32 wait_count;	
	u8 i, res = 0;
#ifndef	DHCP_ENABLE
	uip_ipaddr_t ipaddr;
#endif
		
	res = ENC28J60_Init((u8*)Modbus.mac_addr);	//初始化ENC28J60					  
	//把IP地址和MAC地址写入缓存区
	for(i = 0; i < 6; i++)
	{
		uip_ethaddr.addr[i] = Modbus.mac_addr[i];
	}
    //指示灯状态:0x476 is PHLCON LEDA(绿)=links status, LEDB(红)=receive/transmit
 	//PHLCON：PHY 模块LED 控制寄存器	    
	ENC28J60_PHY_Write(PHLCON, 0x0476);
	uip_init();							//uIP初始化	
	if(Modbus.tcp_type == 0)	
	{
		U8_T temp[4];
		uip_ipaddr(ipaddr, Modbus.ip_addr[0], Modbus.ip_addr[1], Modbus.ip_addr[2], Modbus.ip_addr[3]);	//设置本地设置IP地址
		uip_sethostaddr(ipaddr);					    
		uip_ipaddr(ipaddr, Modbus.getway[0], Modbus.getway[1], Modbus.getway[2], Modbus.getway[3]); 	//设置网关IP地址(其实就是你路由器的IP地址)
		uip_setdraddr(ipaddr);						 
		uip_ipaddr(ipaddr, Modbus.subnet[0], Modbus.subnet[1], Modbus.subnet[2], Modbus.subnet[3]);	//设置网络掩码
		uip_setnetmask(ipaddr);
//		flag_dhcp_configured = 2;

	temp[0] = Modbus.ip_addr[0];
	temp[1] = Modbus.ip_addr[1];
	temp[2] = Modbus.ip_addr[2];
	temp[3] = Modbus.ip_addr[3];
	
	temp[0] |= (255 - Modbus.subnet[0]);
	temp[1] |= (255 - Modbus.subnet[1]);
	temp[2] |= (255 - Modbus.subnet[2]);
	temp[3] |= (255 - Modbus.subnet[3]);
	
	uip_ipaddr(uip_hostaddr_submask,temp[0], temp[1],temp[2] ,temp[3]);
	multicast_addr = temp[3] + (U16_T)(temp[2] << 8) \
		+ ((U32_T)temp[1] << 16) + ((U32_T)temp[0] << 24);
	delay_ms(1);
		
	}
	else  // DHCP
	{
//		flag_dhcp_configured = 0;
		dhcpc_init(Modbus.mac_addr, 6);
		dhcpc_request();	
	}
	
//	printf("res=%u\n\r",res);
	return res;	
}
//读取一包数据  
uint16_t tapdev_read(void)
{	
	return  ENC28J60_Packet_Receive(MAX_FRAMELEN, uip_buf);
}

//发送一包数据  
u8 tapdev_send(void)
{
	return ENC28J60_Packet_Send(uip_len, uip_buf);
}
