#include "main.h"
#include "tcpip.h"
#include "define.h"
#include "resolv.h"


u8 ip_addr[4];
u8 gateway[4];
u16 tcp_port;

U8_T far dyndns_enable;
//U8_T far dyndns_provider;
//U16_T far dyndns_update_time;
uint32 multicast_addr;

STR_SEND_BUF bip_bac_buf;
STR_SEND_BUF bip_bac_buf2;
STR_SEND_BUF mstp_bac_buf[10];
STR_SEND_BUF scan_buf[10];

u8 rec_mstp_index;  // for i-am
u8 send_mstp_index;

u8 rec_mstp_index1; // response packets form   
u8 send_mstp_index1;
STR_SEND_BUF mstp_bac_buf1[10];

//char Send_bip_Flag;
//xSemaphoreHandle Sem_mstp_Flag;
//char Send_mstp_Flag;

#define	BUF	((struct uip_eth_hdr *)&uip_buf[0])	
	
//uint8_t flag_dhcp_configured = 0;

void uip_polling(void);



void tcpip_intial(void)
{
#if 1
	U8_T count = 0;
	u16_t dnsserver[2];
	uip_ipaddr_t ipaddr;
	
	SPI2_Init();
//	Test[11]++;
//	ENC28J60_Reset();
//	delay_ms(1000);
	while(tapdev_init() && (count < 100))	//???ENC28J60??
	{	
		count++;
		delay_ms(50);
	}
//	Test[12]++;
	// <tcp_type == 2> means DCHP fail
	if(Modbus.tcp_type == 0 || Modbus.tcp_type == 2)
	{
		if(Modbus.tcp_type == 2)
		{
			Modbus.tcp_type = 1;
		}
		uip_listen(HTONS(Modbus.tcp_port)); 
		bip_Init();
		udp_scan_init();
		
		dnsserver[0] = (Modbus.getway[1]<<8) + Modbus.getway[0];
		dnsserver[1] = (Modbus.getway[3]<<8) + Modbus.getway[2];
		resolv_init();
		resolv_conf((u16_t *)dnsserver);
//		resolv_query("newfirmware.com");
		if(Modbus.en_sntp > 1)  // 0 - no, 1-disable
			SNTPC_Init();
//		RM_Init();
		if(Modbus.en_dyndns == 2)
		{
			resolv_query("www.3322.org");
			resolv_query("www.dyndns.com");
			resolv_query("www.no-ip.com");	
		}
	
	}

#endif
}




static u16 count_connect = 0;

//u8 myMAC[] = {0x00, 0x0e, 0x00, 0x25, 0x36, 0x64}; 
void TCPIP_Task( void *pvParameters )
{
	uip_ipaddr_t ipaddr;
	u8 count_ip_changed = 0;

	//u8 tcp_server_tsta = 0XFF;
	//u8 tcp_client_tsta = 0XFF;
	
//	delay_ms(500);	
	u8 count = 0 ;
  u32 dhcp_count = 0;
	task_test.enable[0] = 1;
	tcpip_intial();
	count = 0;
	IP_Change = 0;
	watchdog_init(); 

  for( ;; )
	{
		current_task = 0;
		uip_polling();	//??uip??,?????????????? 
		if((flag_reboot == 1)||(update_firmware == 1))
		{
			count++;
			if(count == 10)
			{
				count = 0;
				flag_reboot = 0;
				SoftReset();
			}			
		}		

		
		if(IP_Change)
		{			
			count_ip_changed++;
			if(count_ip_changed > 100)
			{
				count_ip_changed = 0;
				IP_Change = 0;
				QuickSoftReset();
			}
		}
		IWDG_ReloadCounter(); 
		delay_ms(5);
		task_test.count[0]++;
  }
}


//uip�¼�������
//���뽫�ú��������û���ѭ��,ѭ������.
void uip_polling(void)
{
	u8 i;
	static struct timer periodic_timer, arp_timer;
	static u8 timer_ok = 0;	
	if(timer_ok == 0)		//����ʼ��һ��
	{		
		timer_ok = 1;
		timer_set(&periodic_timer, CLOCK_SECOND / 2); //����1��0.5��Ķ�ʱ�� 
		timer_set(&arp_timer, CLOCK_SECOND * 10);	   //����1��10��Ķ�ʱ�� 	}
	}
	Test[10]++;
	uip_len = tapdev_read();							//�������豸��ȡһ��IP��,�õ����ݳ���.uip_len��uip.c�ж���
	if(uip_len > 0)							 			//������
	{  Test[11]++;
		count_connect = 0;
		// receive data
		ether_rx_packet++;
		flagLED_ether_rx = 1;
//		printf("uip_len");
	//����IP���ݰ�(ֻ��У��ͨ����IP���Żᱻ����) 
		if(BUF->type == htons(UIP_ETHTYPE_IP))		//�Ƿ���IP��? 
		{
			uip_arp_ipin();								//ȥ����̫��ͷ�ṹ������ARP��
			uip_input();   									//IP������			
			//������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ��� uip_len > 0
			//��Ҫ���͵�������uip_buf, ������uip_len  (����2��ȫ�ֱ���)		
			if(uip_len > 0)							//��Ҫ��Ӧ����
			{	
				// response data				
				uip_arp_out();							//����̫��ͷ�ṹ������������ʱ����Ҫ����ARP����
				if(tapdev_send() == 1)								//�������ݵ���̫��
				{
					ether_tx_packet++;
					flagLED_ether_tx = 1;
					count_reintial_tcpip = 60; // receive packets and response correctly
				}
			}
		}
		else if (BUF->type == htons(UIP_ETHTYPE_ARP))	//����arp����,�Ƿ���ARP�����?
		{
			uip_arp_arpin();
 			//������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ���uip_len>0
			//��Ҫ���͵�������uip_buf, ������uip_len(����2��ȫ�ֱ���)
 			if(uip_len > 0)
			{
				// response data				
				if(tapdev_send() == 1)								//�������ݵ���̫��
				{
					ether_tx_packet++;
					flagLED_ether_tx = 1;
				}
			}				
		}
	}
	else if(timer_expired(&periodic_timer))				//0.5�붨ʱ����ʱ
	{
		timer_reset(&periodic_timer);					//��λ0.5�붨ʱ�� 
		//��������ÿ��TCP����, UIP_CONNSȱʡ��40��  
		for(i = 0; i < UIP_CONNS; i++)
		{
			uip_periodic(i);							//����TCPͨ���¼�
	 		//������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ���uip_len>0
			//��Ҫ���͵�������uip_buf, ������uip_len (����2��ȫ�ֱ���)
	 		if(uip_len > 0)
			{
				// response data
				uip_arp_out();							//����̫��ͷ�ṹ������������ʱ����Ҫ����ARP����
				if(tapdev_send() == 1)								//�������ݵ���̫��
				{
					ether_tx_packet++;
					flagLED_ether_tx = 1;
					count_reintial_tcpip = 60; // receive tcp packets and response correctly
				}
			}
		}
#if UIP_UDP	//UIP_UDP 
		//��������ÿ��UDP����, UIP_UDP_CONNSȱʡ��10��
		for(i = 0; i < UIP_UDP_CONNS; i++)
		{
			uip_udp_periodic(i);						//����UDPͨ���¼�
	 		//������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ���uip_len>0
			//��Ҫ���͵�������uip_buf, ������uip_len (����2��ȫ�ֱ���)
			if(uip_len > 0)
			{
				// response data
				uip_arp_out();							//����̫��ͷ�ṹ������������ʱ����Ҫ����ARP����
				if(tapdev_send() == 1)								//�������ݵ���̫��
				{
					ether_tx_packet++;
					flagLED_ether_tx = 1;
					count_reintial_tcpip = 60; // receive udp packets and response correctly
				}
			}
		}
#endif 
		//ÿ��10�����1��ARP��ʱ������ ���ڶ���ARP����,ARP��10�����һ�Σ��ɵ���Ŀ�ᱻ����
		if(timer_expired(&arp_timer))
		{
			timer_reset(&arp_timer);
			uip_arp_timer();
		}
	}
}


extern uint8_t PDUBuffer_BIP[MAX_APDU];
//uint8_t far PDUBuffer_BIP1[MAX_APDU];
//uint8_t * bip_Data_client;
//uint16_t  bip_len_client;
//uint16_t bip_receive_client(
//    BACNET_ADDRESS * src,       /* source address */
//    uint8_t * pdu,      /* PDU data */
//    uint16_t max_pdu,   /* amount of space available in the PDU  */
//    unsigned protocal);

// udp client
void remove_bip_client_conn(void);
extern struct uip_udp_conn * bip_send_client_conn;
// for network points
void UDP_bip_send(void)
{
	static u32 t1,t2;
	uip_ipaddr_t addr;
	struct uip_udp_conn *bip_send_conn;

	uint16_t pdu_len = 0;  
	BACNET_ADDRESS far src; /* source address */
	t1 = uip_timer;

	if(t1 - t2 >= 100)
	{
		if(Send_bip_Flag)  // send
		{		
			uip_send(bip_bac_buf.buf,bip_bac_buf.len);	
			Send_bip_Flag = 0;		
			if(Send_bip_count > 0)
				Send_bip_count--;
		}		
		t2 = uip_timer;
	}
	
	if(uip_newdata())
	{  // receive data
		bip_Data = PDUBuffer_BIP;
		memcpy(bip_Data,uip_appdata,uip_len);
		bip_len = uip_len;
		uip_ipaddr_copy(addr, uip_udp_conn->ripaddr);
		bip_send_conn = uip_udp_new(&addr, uip_udp_conn->rport);
		pdu_len = bip_receive(&src, &PDUBuffer_BIP[0], MAX_APDU,BAC_IP);
	  {
			if(pdu_len) 
			{ 
				npdu_handler(&src, &PDUBuffer_BIP[0], pdu_len, BAC_IP);	
			}			
		}
		uip_udp_remove(bip_send_conn);
	}
	
}

// for COV

void UDP_bip_send2(void)
{
	static u32 t1,t2;
	uip_ipaddr_t addr;
//	struct uip_udp_conn *bip_send_conn;

	uint16_t pdu_len = 0;  
	BACNET_ADDRESS far src; /* source address */
	t1 = uip_timer;
	if(t1 - t2 >= 100)
	{
		if(Send_bip_Flag2)  // send
		{
			uip_send(bip_bac_buf2.buf,bip_bac_buf2.len);	
			Send_bip_Flag2 = 0;		
			if(Send_bip_count2 > 0)
				Send_bip_count2--;
		}		
		t2 = uip_timer;
	}
	
	/*if(uip_newdata())
	{  // receive data
		bip_Data = PDUBuffer_BIP;
		memcpy(bip_Data,uip_appdata,uip_len);
		bip_len = uip_len;
		uip_ipaddr_copy(addr, uip_udp_conn->ripaddr);
		bip_send_conn = uip_udp_new(&addr, uip_udp_conn->rport);
		pdu_len = bip_receive(&src, &PDUBuffer_BIP[0], MAX_APDU,BAC_IP);
	  {
			if(pdu_len) 
			{ 
				npdu_handler(&src, &PDUBuffer_BIP[0], pdu_len, BAC_IP);	
			}			
		}
		uip_udp_remove(bip_send_conn);
	}*/
	
}
	
//int8_t Get_uip_udp_conn_pos(uip_ipaddr_t *lripaddr, u16_t lrport)
//{
//	char c;
//	struct uip_udp_conn * conn;
//	for(c = 0; c < UIP_UDP_CONNS; ++c) 
//	{
//    conn = &uip_udp_conns[c];
//	
//		// udp client
//		if((memcmp(conn->ripaddr,lripaddr,4) == 0)  
//			&& (conn->rport == lrport)
//		)
//		{		
//			return c;
//		}
//		
//		// udp server
//		if((memcmp(conn->ripaddr,lripaddr,4) == 0)  
//			&& (conn->lport == lrport)
//		)
//		{
//			return c;
//		}
//	}
//	
//	return -1;
//}

struct uip_udp_conn * bacnet_Rec_conn;
extern U32_T response_bacnet_ip;
extern U32_T response_bacnet_port;
void Response_bacnet_Start(void)
{
	uip_ipaddr_t addr;
	
	if(bacnet_Rec_conn != NULL) 
	{
		uip_udp_remove(bacnet_Rec_conn);
	}

	uip_ipaddr(addr, (U8_T)(response_bacnet_ip), (U8_T)(response_bacnet_ip >> 8), (U8_T)(response_bacnet_ip >> 16), (U8_T)(response_bacnet_ip >> 24));	
	bacnet_Rec_conn = uip_udp_new(&addr, HTONS(response_bacnet_port));
	if(bacnet_Rec_conn != NULL) { 
		uip_udp_bind(bacnet_Rec_conn, HTONS(Modbus.Bip_port/*UDP_BACNET_LPORT*/));
		send_mstp_index = 0;
			
  }

}

void Response_bacnet_appcall(void)  // 47808
{
	static u32 t1 = 0;
	static u32 t2 = 0;

	if(uip_poll()) 
	{	
		// auto send
		t1 = uip_timer;			
		
		if(t1 - t2 >= 100)
		{	
			if(send_mstp_index < rec_mstp_index)
			{
				uip_send(mstp_bac_buf[send_mstp_index].buf,mstp_bac_buf[send_mstp_index].len);	
				send_mstp_index++;
			}	
			t2 = uip_timer;
		}
		
	}
}


extern u32 response_scan_ip;
extern u32 response_scan_port;
void udp_demo_appcall(void)
{
// udp server
//	char pos = -1;
//	
//	pos = -1;	

	
	if((uip_udp_conn->lport == HTONS(UDP_SCAN_LPORT)) && (uip_udp_conn->lport != 0))
	{
		//		pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->lport);
		Response_Scan_appcall();
	}
	
	if((uip_udp_conn->lport == HTONS(Modbus.Bip_port/*UDP_BACNET_LPORT*/)) && (uip_udp_conn->lport != 0)
		&& (uip_udp_conn->rport == HTONS(response_bacnet_port)))
	{
		Response_bacnet_appcall();
	}
	
	if(uip_udp_conn->lport == HTONS(Modbus.Bip_port/*UDP_BACNET_LPORT*/))
	{
		if(Send_bip_Flag2)
		{
			UDP_bip_send2();
		}
		else
		{
			UDP_bacnet_APP();
		}
	}
	switch(uip_udp_conn->lport)
	{
	case HTONS(UDP_SCAN_LPORT):
//			pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->lport);
		UDP_SCAN_APP();			
		break;
	/*case HTONS(UDP_BACNET_LPORT):
		// get ip and port 
//		pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->lport);
		if(Send_bip_Flag2)
		{
			UDP_bip_send2();
		}
		else
		{
			UDP_bacnet_APP();
		}
		break;*/
	case HTONS(UDP_BIP_SEND_LPORT):
//		pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->lport);

		UDP_bip_send();
		break;
	default: 

		break;
	}

// udp client	
	switch(uip_udp_conn->rport)
	{
		case HTONS(67): // DHCP SERVER PORT
//			pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->rport);
			dhcpc_appcall();
			break;
//		case HTONS(68):
//			dhcpc_appcall();
//			break;
		case HTONS(53):  // DNS SERVER PORT
//			pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->rport);
			resolv_appcall();
			break;
		case HTONS(123):// SNTP SERVER PORT
//			pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->rport);
			sntp_appcall();
			break;
		case HTONS(80):// DYNDNS SERVER PORT
//			pos = Get_uip_udp_conn_pos(&uip_udp_conns->ripaddr,uip_udp_conns->rport);
			break;
		case HTONS(SERVER_HEARTBEAT_PORT):
			//RM_Heart_appcall();
		break;
		case HTONS(SERVER_MINI_PORT):
			//RM_Rec_appcall();
		break;
		default:
	
			break;
	}
		
}
void DynDNS_appcall(void);
extern U32_T far dyndns_HostIp;

uint8_t Modbus_Client_State;
uint8_t flag_Modbus_Client_Send;


#define Modbus_STATE_IDLE				0
#define Modbus_STATE_MAKE_CONNECT		1
#define Modbus_STATE_WAIT_CONNECT		2
#define Modbus_STATE_SEND_COMMAND		3
#define Modbus_STATE_WAIT_RESPONSE		4
#define Modbus_STATE_UPDATE_OK		5  // added by chelsea



uint8_t Modbus_Client_Command[UIP_CONNS][20];
uint8_t Modbus_Client_CmdLen;
uint8_t tcp_client_sta[UIP_CONNS];
S8_T get_netpoint_index_by_invoke_id_modbus(uint8_t invokeid);
S8_T get_netpoint_index(uint8_t ip,uint8_t id,uint8_t reg);
int8_t Get_uip_conn_pos(uip_ipaddr_t *lripaddr, u16_t lrport);
extern u8_t uip_time_to_live[UIP_CONF_MAX_LISTENPORTS];
extern int8_t conn_pos;

#if NETWORK_MODBUS
extern xSemaphoreHandle sem_Modbus_Client_Send;
void Modbus_Client_appcall(struct uip_conn * conn)
{
	//struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;
	uint32_t ip;
	S8_T far network_point_index;	
	char i;
	memcpy(&ip,conn->ripaddr,4);
	
	if(uip_poll()) 
	{
		//if(flag_Modbus_Client_Send == 0)  Test[25]++;
		//{
		//	flag_Modbus_Client_Send = 0;
			for(i = 0;i < UIP_CONNS;i++)
			{
				if(Modbus_Client_Command[i][0] == (U8_T)(ip >> 24))
				{
					uip_send(Modbus_Client_Command[i], Modbus_Client_CmdLen);
					break;				
				}
			}	
		//}
	}
	
	
	// if tcp server is deleted
	if(uip_aborted()) 	{conn->tcpstateflags = UIP_CLOSED; 

	}	//������ֹ	   
	if(uip_timedout())   conn->tcpstateflags = UIP_CLOSED;	
	//���ӳ�ʱ   
	if(uip_closed())  	 conn->tcpstateflags = UIP_CLOSED;	
		//���ӹر�
	
 	if(uip_connected()) //���ӳɹ�	
	{
		conn->tcpstateflags = UIP_ESTABLISHED;
		conn_pos = Get_uip_conn_pos(&conn->ripaddr,conn->rport);

	}   

		
 	//���յ�һ���µ�TCP���ݰ� 
	if(uip_newdata())
	{
			U8_T tcp_clinet_buf[20];
			float val_ptr;
			// add value
			U8_T float_type;
		
			if(uip_len == 11 || uip_len == 13 || uip_len == 10)  // response read
			{ // READ ONE is 11, read 2bytes is 13, read coil is 10
				memcpy(&tcp_clinet_buf, uip_appdata,uip_len);		
				//invokeid = tcp_clinet_buf[5];
				
				network_point_index = get_netpoint_index_by_invoke_id_modbus(tcp_clinet_buf[1]);
				if(network_point_index == -1) return;
				//if(network_points_list_modbus[network_point_index].point.panel == (U8_T)(ip >> 24) )
				float_type = (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;				
				if((U8_T)(ip >> 24) == tcp_clinet_buf[0])
				{
					if(uip_len == 13)	// read input float 32bit
						float_type = 1;
					if(float_type == 1)
						val_ptr = (U32_T)(tcp_clinet_buf[9] << 24) + (U32_T)(tcp_clinet_buf[10] << 16) \
									+ (U16_T)(tcp_clinet_buf[11] << 8) + tcp_clinet_buf[12];
					else
					{
						if(uip_len == 11)
							val_ptr = tcp_clinet_buf[9] * 256 + tcp_clinet_buf[10];
						else if(uip_len == 10)
							val_ptr = tcp_clinet_buf[9];
						else
							;// error
					}

					if((tcp_clinet_buf[6] == network_points_list_modbus[network_point_index].tb.NT_modbus.id) 
						&& (network_points_list_modbus[network_point_index].tb.NT_modbus.id != 0)
						&& (tcp_clinet_buf[7] == (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0x7f))
					)
					{
						
						add_network_point( network_points_list_modbus[network_point_index].point.panel,
						network_points_list_modbus[network_point_index].point.sub_id,
						network_points_list_modbus[network_point_index].point.point_type - 1,
						network_points_list_modbus[network_point_index].point.number + 1,
						val_ptr,
						0,float_type);
						flag_receive_netp_modbus = 1;
						network_points_list_modbus[network_point_index].lose_count = 0;
						network_points_list_modbus[network_point_index].decomisioned = 1;
					}
				}
			}
			else if(uip_len == 12 || uip_len == 17)	// response write
			{
				memcpy(&tcp_clinet_buf, uip_appdata,uip_len);	
				flag_receive_netp_modbus = 1;
				
				network_point_index = get_netpoint_index_by_invoke_id_modbus(tcp_clinet_buf[1]);
				float_type = (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;	
				
				if(network_point_index == -1) return;
				
				if((U8_T)(ip >> 24) == tcp_clinet_buf[0])
				{
					if(float_type == 0)
					{
						val_ptr = tcp_clinet_buf[10] * 256 + tcp_clinet_buf[11];			
						
						add_network_point( network_points_list_modbus[network_point_index].point.panel,
								network_points_list_modbus[network_point_index].point.sub_id,
								network_points_list_modbus[network_point_index].point.point_type - 1,
								network_points_list_modbus[network_point_index].point.number + 1,
								val_ptr,
						0,float_type);
					}
					
				}			
						
			}
	}
		
}
#endif


void tcp_appcall(void)
{	
	uip_ipaddr_t addr;
	int8_t pos = -1;
	
	pos = -1;

	// tcp server
	if(uip_conn->lport == HTONS(Modbus.tcp_port))  // maybe need change bigsamll end
	{	
		tcp_server_appcall(uip_conn); 
		pos = Get_uip_conn_pos(&uip_conn->ripaddr,uip_conn->lport);
		if(pos >= 0)
		{
			uip_time_to_live[pos] = 20;
		}
	}
	
	// tcp client
	if(uip_conn->rport == HTONS(80))
	{
		uip_ipaddr(&addr,(U8_T)(dyndns_server_ip >> 24), (U8_T)(dyndns_server_ip >> 16), (U8_T)(dyndns_server_ip >> 8), (U8_T)(dyndns_server_ip));	
		if(uip_ipaddr_cmp(&addr,uip_conn->ripaddr))
		{
			
		}
		
		DynDNS_appcall();
		if(pos >= 0)
		{
			if(uip_newdata())
				uip_time_to_live[pos] = 20;
		}
	}
	
#if SMTP	
	// add SMTP
	// tcp client
	if(uip_conn->rport == HTONS(SMTP_SERVER_PORT))
	{
		uip_ipaddr(&addr,(U8_T)(smtpc_Conns.ServerIp >> 24), (U8_T)(smtpc_Conns.ServerIp >> 16), (U8_T)(smtpc_Conns.ServerIp >> 8), (U8_T)(smtpc_Conns.ServerIp));	
		if(uip_ipaddr_cmp(&addr,uip_conn->ripaddr))
		{
			
		}
		
		SMTPC_appcall();
//		if(pos >= 0)
//		{
//			if(uip_newdata())
//				uip_time_to_live[pos] = 20;
//		}
		
		
	}
#endif
	
	#ifdef ETHERNET_DEBUG
	if(uip_conn->rport == HTONS(1115))
	{
		uip_ipaddr(&addr,(U8_T)(192), (U8_T)(168), (U8_T)(0), (U8_T)(38));	
		if(uip_ipaddr_cmp(&addr,uip_conn->ripaddr))
		{
			
		}
		EthernetDebug_appcall();
	}
	#endif
	
	
#if NETWORK_MODBUS

	if(uip_conn->rport == HTONS(502))
	{
		Modbus_Client_appcall(uip_conn);
		pos = Get_uip_conn_pos(&uip_conn->ripaddr,uip_conn->rport);
		if(pos >= 0)
			{
				if(uip_newdata())
					uip_time_to_live[pos] = 20;
			}
	}
#endif


}

void dhcpc_configured(const struct dhcpc_state *s)
{
	U8_T temp[4];
	uip_sethostaddr(s->ipaddr);
	uip_setnetmask(s->netmask);
	uip_setdraddr(s->default_router);
	resolv_init();
	resolv_conf((u16_t *)s->dnsaddr);
//	resolv_query("newfirmware.com");
	Test[32]++;
	uip_listen(HTONS(Modbus.tcp_port)); 
	if(Modbus.en_sntp > 1)
		SNTPC_Init();
//	RM_Init();
	if(Modbus.en_dyndns == 2)
	{
		resolv_query("www.3322.org");
		resolv_query("www.dyndns.com");
		resolv_query("www.no-ip.com");
	}		
	
	if(memcmp(Modbus.ip_addr,uip_hostaddr,4) && !((uip_ipaddr1(uip_hostaddr) == 0) && (uip_ipaddr2(uip_hostaddr) == 0) \
	&& (uip_ipaddr3(uip_hostaddr)== 0) && (uip_ipaddr4(uip_hostaddr) == 0)))
	{
		Modbus.ip_addr[0] = uip_ipaddr1(uip_hostaddr);
		Modbus.ip_addr[1] = uip_ipaddr2(uip_hostaddr);
		Modbus.ip_addr[2] = uip_ipaddr3(uip_hostaddr);
		Modbus.ip_addr[3] = uip_ipaddr4(uip_hostaddr);

//		memcpy(own_ip,Modbus.ip_addr,4);
			
		E2prom_Write_Byte(EEP_IP + 0, Modbus.ip_addr[0]);
		E2prom_Write_Byte(EEP_IP + 1, Modbus.ip_addr[1]);
		E2prom_Write_Byte(EEP_IP + 2, Modbus.ip_addr[2]);
		E2prom_Write_Byte(EEP_IP + 3, Modbus.ip_addr[3]);

	}

	if(memcmp(Modbus.subnet,uip_netmask,4) && !((uip_ipaddr1(uip_netmask) == 0) && (uip_ipaddr2(uip_netmask) == 0) \
				&& (uip_ipaddr3(uip_netmask) == 0) && (uip_ipaddr4(uip_netmask) == 0)))
	{	
		Modbus.subnet[0] = uip_ipaddr1(uip_netmask);
		Modbus.subnet[1] = uip_ipaddr2(uip_netmask);
		Modbus.subnet[2] = uip_ipaddr3(uip_netmask);
		Modbus.subnet[3] = uip_ipaddr4(uip_netmask);
		
		E2prom_Write_Byte(EEP_SUBNET + 0, Modbus.subnet[0]);
		E2prom_Write_Byte(EEP_SUBNET + 1, Modbus.subnet[1]);
		E2prom_Write_Byte(EEP_SUBNET + 2, Modbus.subnet[2]);
		E2prom_Write_Byte(EEP_SUBNET + 3, Modbus.subnet[3]);
	}	

	if(memcmp(Modbus.getway,uip_draddr,4) && !((uip_ipaddr1(uip_draddr) == 0) && (uip_ipaddr2(uip_draddr) == 0) \
		&& (uip_ipaddr3(uip_draddr) == 0) && (uip_ipaddr4(uip_draddr) == 0)))	
	{
		Modbus.getway[0] = uip_ipaddr1(uip_draddr);
		Modbus.getway[1] = uip_ipaddr2(uip_draddr);
		Modbus.getway[2] = uip_ipaddr3(uip_draddr);
		Modbus.getway[3] = uip_ipaddr4(uip_draddr);
		
		E2prom_Write_Byte(EEP_GETWAY + 0, Modbus.getway[0]);
		E2prom_Write_Byte(EEP_GETWAY + 1, Modbus.getway[1]);
		E2prom_Write_Byte(EEP_GETWAY + 2, Modbus.getway[2]);
		E2prom_Write_Byte(EEP_GETWAY + 3, Modbus.getway[3]);
	}

		
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
	bip_Init();
	udp_scan_init();
//	flag_dhcp_configured = 1;
//	Test[30]++;
}



