/*
 * uart_base.cpp
 *
 *  Created on: 2016年12月21日
 *      Author: user
 */


#include<stdio.h>      /*标准输入输出定义*/
#include<stdlib.h>     /*标准函数库定义*/
#include<unistd.h>     /*Unix 标准函数定义*/
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>      /*文件控制定义*/
#include<termios.h>    /*PPSIX 终端控制定义*/
#include<errno.h>      /*错误号定义*/
#include<string.h>
#include "uart_base.h"
uart_base::uart_base() {
	// TODO Auto-generated constructor stub

}

uart_base::~uart_base() {
	// TODO Auto-generated destructor stub
}


/**
 * 功能: 打开@uartPort 指定的串口 并且返回 fd
 * 返回值: 如果打开成功返回fd, 如果打开失败返回-1
 *
 * */
int uart_base::UARTOpen(char* uartPort){

	int fd = open(uartPort, O_RDWR|O_NOCTTY|O_NDELAY);
	if(-1 == fd){
		printf("Open Uart %s failed\n", uartPort);
		return -1;
	}

	//判断串口的状态是否为阻塞状态
	if(fcntl(fd, F_SETFL, 0) < 0){
		printf("fcntl failed!\n");
		return -1;
	}else{
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
	}

	//测试是否为终端设备
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
 * 函数功能:设置串口相关参数
 * 参数：	fd 文件描述符
 * 		speed 串口速度
 * 		flow_ctrl 数据流控
 * 		databits 数据位 取值为7或者8
 * 		stopbits 停止位 取值为1或者2
 * 		parity 校验类型 取值为N,E,O,S
 *返回值: 正确返回0 错误返回-1
 * */
int uart_base::UARTSet(int fd, unsigned int speed, int flow_ctrl, int databits, int stopbits, int parity){
	int i;
	int status;
	unsigned int speed_arr[] = {B230400,  B115200,  B57600,  B38400,  B19200,  B9600,  B4800, B2400, B1800};
	unsigned int name_arr[] = {230400,  115200,  57600,  38400,  19200,  9600,  4800, 2400, 1800};
	struct termios options;

	// tcgetattr获取fd指向对象的相关参数 并进行保存 还可以测试配置是否真确
	if(tcgetattr(fd, &options) != 0){
		printf("Get attr error\n");
		return -1;
	}

	//设置串口输入波特率和输出波特率
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++){
		if  (speed == name_arr[i]){
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
		}
	}

	//修改控制模式，保证程序不会占用串口
	options.c_cflag |= CLOCAL;
	//修改控制模式，使得能够从串口中读取输入数据
	options.c_cflag |= CREAD;

	//设置数据流控制
	switch(flow_ctrl) {
	   case 0 ://不使用流控制
			  options.c_cflag &= ~CRTSCTS;
			  break;
	   case 1 ://使用硬件流控制
			  options.c_cflag |= CRTSCTS;
			  break;
	   case 2 ://使用软件流控制
			  options.c_cflag |= IXON | IXOFF | IXANY;
			  break;
	   case 3:
		   	   options.c_iflag = 0;
		   	   options.c_oflag = 0;
		   	   break;
	}

	//设置数据位
	//屏蔽其他标志位
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

   //设置校验位
	switch (parity){
	   case 'n':
	   case 'N': //无奇偶校验位。
			 options.c_cflag &= ~PARENB;
			 options.c_iflag &= ~INPCK;
			 break;
	   case 'o':
	   case 'O'://设置为奇校验
			 options.c_cflag |= (PARODD | PARENB);
			 options.c_iflag |= INPCK;
			 break;
	   case 'e':
	   case 'E'://设置为偶校验
			 options.c_cflag |= PARENB;
			 options.c_cflag &= ~PARODD;
			 options.c_iflag |= INPCK;
			 break;
	   case 's':
	   case 'S': //设置为空格
			 options.c_cflag &= ~PARENB;
			 options.c_cflag &= ~CSTOPB;
			 break;
		default:
			 printf("Unsupported parity\n");
			 return (-1);
	}

	// 设置停止位
	switch (stopbits){
	   case 1:
			 options.c_cflag &= ~CSTOPB; break;
	   case 2:
			 options.c_cflag |= CSTOPB; break;
	   default:
		   printf("Unsupported stop bits\n");
		   return (-1);
	}


	//修改输出模式，原始数据输出
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//我加的
	//options.c_lflag &= ~(ISIG | ICANON);

	//设置等待时间和最小接收字符
	options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
	options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */

	//如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
	tcflush(fd,TCIFLUSH);

	//激活配置 (将修改后的termios数据设置到串口中）
	if (tcsetattr(fd,TCSANOW,&options) != 0){
		printf("com set error!\n");
	    return (-1);
	}

	return 0;

}

/**
 * 功能：接受@fd对于的串口的数据 每次接受 传入@rcb_buf作为缓存，最大长度为@buf_len指定
 * 返回值: 返回实际接受串口数据大小
 * */
int uart_base::UART_Recv(int fd, unsigned char* rcv_buf, unsigned int buf_len){

	int len,fs_sel;
	fd_set fs_read;

	struct timeval time;

	FD_ZERO(&fs_read);
	FD_SET(fd,&fs_read);

	time.tv_sec = 0;
	time.tv_usec = 500000;

	//使用select实现串口的多路通信
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

