/*
 * uart_base.h
 *
 *  Created on: 2016年12月21日
 *      Author: user
 */

#ifndef UART_BASE_H_
#define UART_BASE_H_

class uart_base {					//串口的基础功能包
public:
	uart_base();
	virtual ~uart_base();

	int UARTOpen(char* uartPort);
	void UARTClose(int fd);
	int UARTSet(int fd,unsigned int speed, int flow_ctrl, int databits, int stopbits, int parity);
	int UART_Recv(int fd, unsigned char *rcv_buf, unsigned int data_len);
	int UART_Send(int fd,void *send_buf,unsigned int data_len);
};




#endif /* UART_BASE_H_ */
