/*
 * uart_base.cpp
 *
 *  Created on: 2016��12��21��
 *      Author: user
 */


#include<stdio.h>      /*��׼�����������*/
#include<stdlib.h>     /*��׼�����ⶨ��*/
#include<unistd.h>     /*Unix ��׼��������*/
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>      /*�ļ����ƶ���*/
#include<termios.h>    /*PPSIX �ն˿��ƶ���*/
#include<errno.h>      /*����Ŷ���*/
#include<string.h>
#include "uart_base.h"
uart_base::uart_base() {
	// TODO Auto-generated constructor stub

}

uart_base::~uart_base() {
	// TODO Auto-generated destructor stub
}


/**
 * ����: ��@uartPort ָ���Ĵ��� ���ҷ��� fd
 * ����ֵ: ����򿪳ɹ�����fd, �����ʧ�ܷ���-1
 *
 * */
int uart_base::UARTOpen(char* uartPort){

	int fd = open(uartPort, O_RDWR|O_NOCTTY|O_NDELAY);
	if(-1 == fd){
		printf("Open Uart %s failed\n", uartPort);
		return -1;
	}

	//�жϴ��ڵ�״̬�Ƿ�Ϊ����״̬
	if(fcntl(fd, F_SETFL, 0) < 0){
		printf("fcntl failed!\n");
		return -1;
	}else{
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
	}

	//�����Ƿ�Ϊ�ն��豸
	if(0 == isatty(fd)){
		printf("%s is not a termimal device\n", uartPort);
		return -1;
	}else{
		printf("isatty success!\n");
	}

	return fd;
}


void uart_base::UARTClose(int fd){
	close(fd);
}


/**
 * ��������:���ô�����ز���
 * ������	fd �ļ�������
 * 		speed �����ٶ�
 * 		flow_ctrl ��������
 * 		databits ����λ ȡֵΪ7����8
 * 		stopbits ֹͣλ ȡֵΪ1����2
 * 		parity У������ ȡֵΪN,E,O,S
 *����ֵ: ��ȷ����0 ���󷵻�-1
 * */
int uart_base::UARTSet(int fd, unsigned int speed, int flow_ctrl, int databits, int stopbits, int parity){
	int i;
	int status;
	unsigned int speed_arr[] = {B230400,  B115200,  B57600,  B38400,  B19200,  B9600,  B4800, B2400, B1800};
	unsigned int name_arr[] = {230400,  115200,  57600,  38400,  19200,  9600,  4800, 2400, 1800};
	struct termios options;

	// tcgetattr��ȡfdָ��������ز��� �����б��� �����Բ��������Ƿ���ȷ
	if(tcgetattr(fd, &options) != 0){
		printf("Get attr error\n");
		return -1;
	}

	//���ô������벨���ʺ����������
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++){
		if  (speed == name_arr[i]){
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
		}
	}

	//�޸Ŀ���ģʽ����֤���򲻻�ռ�ô���
	options.c_cflag |= CLOCAL;
	//�޸Ŀ���ģʽ��ʹ���ܹ��Ӵ����ж�ȡ��������
	options.c_cflag |= CREAD;

	//��������������
	switch(flow_ctrl) {
	   case 0 ://��ʹ��������
			  options.c_cflag &= ~CRTSCTS;
			  break;
	   case 1 ://ʹ��Ӳ��������
			  options.c_cflag |= CRTSCTS;
			  break;
	   case 2 ://ʹ�����������
			  options.c_cflag |= IXON | IXOFF | IXANY;
			  break;
	   case 3:
		   	   options.c_iflag = 0;
		   	   options.c_oflag = 0;
		   	   break;
	}

	//��������λ
	//����������־λ
	options.c_cflag &= ~CSIZE;
	switch (databits)
	{
	   case 5:
		 options.c_cflag |= CS5;
		 break;
	   case 6:
		 options.c_cflag |= CS6;
		 break;
	   case 7:
		 options.c_cflag |= CS7;
		 break;
	   case 8:
		 options.c_cflag |= CS8;
		 break;
	   default:
		 printf("Unsupported data size\n");
		 return -1;
	}

   //����У��λ
	switch (parity){
	   case 'n':
	   case 'N': //����żУ��λ��
			 options.c_cflag &= ~PARENB;
			 options.c_iflag &= ~INPCK;
			 break;
	   case 'o':
	   case 'O'://����Ϊ��У��
			 options.c_cflag |= (PARODD | PARENB);
			 options.c_iflag |= INPCK;
			 break;
	   case 'e':
	   case 'E'://����ΪżУ��
			 options.c_cflag |= PARENB;
			 options.c_cflag &= ~PARODD;
			 options.c_iflag |= INPCK;
			 break;
	   case 's':
	   case 'S': //����Ϊ�ո�
			 options.c_cflag &= ~PARENB;
			 options.c_cflag &= ~CSTOPB;
			 break;
		default:
			 printf("Unsupported parity\n");
			 return (-1);
	}

	// ����ֹͣλ
	switch (stopbits){
	   case 1:
			 options.c_cflag &= ~CSTOPB; break;
	   case 2:
			 options.c_cflag |= CSTOPB; break;
	   default:
		   printf("Unsupported stop bits\n");
		   return (-1);
	}


	//�޸����ģʽ��ԭʼ�������
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//�Ҽӵ�
	//options.c_lflag &= ~(ISIG | ICANON);

	//���õȴ�ʱ�����С�����ַ�
	options.c_cc[VTIME] = 1; /* ��ȡһ���ַ��ȴ�1*(1/10)s */
	options.c_cc[VMIN] = 1; /* ��ȡ�ַ������ٸ���Ϊ1 */

	//�����������������������ݣ����ǲ��ٶ�ȡ ˢ���յ������ݵ��ǲ���
	tcflush(fd,TCIFLUSH);

	//�������� (���޸ĺ��termios�������õ������У�
	if (tcsetattr(fd,TCSANOW,&options) != 0){
		printf("com set error!\n");
	    return (-1);
	}

	return 0;

}

/**
 * ���ܣ�����@fd���ڵĴ��ڵ����� ÿ�ν��� ����@rcb_buf��Ϊ���棬��󳤶�Ϊ@buf_lenָ��
 * ����ֵ: ����ʵ�ʽ��ܴ������ݴ�С
 * */
int uart_base::UART_Recv(int fd, unsigned char* rcv_buf, unsigned int buf_len){

	int len,fs_sel;
	fd_set fs_read;

	struct timeval time;

	FD_ZERO(&fs_read);
	FD_SET(fd,&fs_read);

	time.tv_sec = 0;
	time.tv_usec = 500000;

	//ʹ��selectʵ�ִ��ڵĶ�·ͨ��
	fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
	if(fs_sel){
		len = read(fd, rcv_buf, buf_len);
		return len;
	}else{
		//printf("Sorry,I am wrong!\n");
		return -1;
	}

}

int uart_base::UART_Send(int fd,void *send_buf,unsigned int data_len){
	int len = 0;

	len = write(fd,send_buf,data_len);
	if (len == data_len ){
		return len;
	}else{
		tcflush(fd,TCOFLUSH);
		return 0;
	}

}

