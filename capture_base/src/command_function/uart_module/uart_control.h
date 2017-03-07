/*
 * uart_control.h
 *
 *  Created on: 2016年12月19日
 *      Author: user
 */

#ifndef UART_CONTROL_H_
#define UART_CONTROL_H_


#include <iostream>
#include "uart_base.h"

#define DEBUG_UART		"/dev/ttymxc0"
#define	CMD_UART		"/dev/ttymxc1"
#define FPGA_UART		"/dev/ttymxc2"



#define Recv_head_size    		2
#define Recv_total_size    		46

#define Send_head_size    		2
#define Send_total_size    		12

#define camera_cmd_size 		12

using namespace std;




class uart_message_deal:public uart_base{
public:
	uart_message_deal();				//Use CMD_UART
	~uart_message_deal();				//Use CMD_UART
	int recv_from_cmd_uart(void);				//从用户接收数据	// 0xEB 0x90 0x90 +43
	int deal_message(void);
	int fixed_time_return(unsigned int left_ssd_size,int now_state,unsigned int now_x,unsigned int now_y,unsigned int now_frame_rates);

	int cmd_uart_fd;
	int to_fpga_fd;
	unsigned char Recv_cmd_buf[Recv_total_size];			//接受的串口数据
	unsigned char Send_cmd_buf[Send_total_size];			//接受的串口数据
private:
	unsigned char Recv_Header[Recv_head_size];			//串口头信息
	unsigned char Send_Header[Send_head_size];			//串口头信息
	unsigned char to_camera_cmd[camera_cmd_size];			//转发给FPGA的命令
};

extern uart_message_deal *uart_cotrol_p;

#endif /* UART_CONTROL_H_ */
