/*
 * uart.cpp
 *
 *  Created on: 2016年12月19日
 *      Author: user
 */


/*
 * UartController.cpp
 *
 *  Created on: 2016-1-13
 *      Author: WJ
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
#include "uart_control.h"
#include "../command.h"

using namespace std;

uart_message_deal *uart_cotrol_p=NULL;

//char set_camera_width_512[12]={0x01,0x01,0x04,0x04,0x02,0x03,0x00,0x00,0x02,0x00,0x00,0x03};
//char set_camera_width_1024[12]={0x01,0x01,0x04,0x04,0x02,0x03,0x00,0x00,0x04,0x00,0x00,0x03};
//char set_camera_length_512[12]={0x01,0x01,0x04,0x24,0x02,0x03,0x00,0x00,0x02,0x00,0x00,0x03};
//char set_camera_length_1024[12]={0x01,0x01,0x04,0x24,0x02,0x03,0x00,0x00,0x04,0x00,0x00,0x03};

char set_camera_manual_gain[12]={0x01,0x01,0x04,0x04,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x03};		//手动增益
char set_camera_manual_exposure[12]={0x01,0x01,0x04,0x24,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x03};	//手动曝光

uart_message_deal::uart_message_deal(){
	int ret;
	cmd_uart_fd=UARTOpen(CMD_UART);
	if(cmd_uart_fd == -1){
		printf("UARTOpen Error\r\n");
	}
	/* 设置串口1 */
	ret = UARTSet(cmd_uart_fd,115200, 3, 8 , 1, 'n');
	if(ret == -1){
		printf("UARTSet Error\r\n");
	}

	to_fpga_fd=UARTOpen(FPGA_UART);
	if(to_fpga_fd == -1){
		printf("UARTOpen Error\r\n");
	}
	ret = UARTSet(to_fpga_fd,9600, 3, 8 , 1, 'n');
	if(ret == -1){
		printf("UARTSet Error\r\n");
	}
	cout<<"Init uart_message_deal________ "<<CMD_UART<<" and "<<FPGA_UART<<" are opened !"<<endl;
		Recv_Header[0]=0xEB;
		Recv_Header[1]=0x90;

		// 0x55 0xaa 0xff 0xcmd +12byte
		Send_cmd_buf[0]=0x55;
		Send_cmd_buf[1]=0xAA;

		to_camera_cmd[0]=0x01;
		to_camera_cmd[1]=0x01;
		to_camera_cmd[2]=0x04;

		to_camera_cmd[11]=0x03;

}

uart_message_deal::~uart_message_deal(){
	UARTClose(cmd_uart_fd);
	UARTClose(to_fpga_fd);
	cout<<"Exit uart_message_deal________"<<endl;

}


int uart_message_deal::recv_from_cmd_uart(void)				//从用户接收数据
{
	int ret;
	int cmd;
	unsigned char commandReadTmp[1];
	unsigned int sum=0;
	int currentIndex = 0;

	while(1){
		ret =UART_Recv(cmd_uart_fd, commandReadTmp, 1);
		if(ret == 1){
//			printf("currentIndex = %d commandReadTmp[0] = %x \r\n", currentIndex, commandReadTmp[0]);
			if(currentIndex < Recv_head_size){
				if(commandReadTmp[0] == Recv_Header[currentIndex]){
					Recv_cmd_buf[currentIndex] = commandReadTmp[0];
//					printf("commandBuffer[%d] = %x \r\n", currentIndex, commandBuffer[currentIndex]);
					currentIndex++;
				}else{
					currentIndex=0;
				}
			}else{
				Recv_cmd_buf[currentIndex] = commandReadTmp[0];
//				printf("commandBuffer[%d] = %x \r\n", currentIndex, commandBuffer[currentIndex]);
				currentIndex++;
			}
			if(currentIndex == Recv_total_size){
				for(int i = 0; i < (Recv_total_size-1); i++){
					sum += Recv_cmd_buf[i];
//					printf("**************sum = 0X%x \n",sum);
//					printf("Recv_total_size [%d]= 0X%x \n",i,Recv_cmd_buf[i]);
				}
				if(Recv_cmd_buf[45]==(sum &0xFF)){
					cmd=Recv_cmd_buf[2];  //命令是[2]
					switch(cmd){				//3 4 5 6 为数据位，to_camera_cmd  7 8 9 10 为
					case 0x90:				//宽
						to_camera_cmd[3]=0x04;
						to_camera_cmd[4]=0x02;
						to_camera_cmd[5]=0x03;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						cmd= REFRESH_HEAD;
						break;
					case 0x91:				//长
						to_camera_cmd[3]=0x24;
						to_camera_cmd[4]=0x02;
						to_camera_cmd[5]=0x03;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						cmd= REFRESH_HEAD;
						break;
					case 0x92:				//手动增益后的,增益设置
						to_camera_cmd[3]=0x04;
						to_camera_cmd[4]=0x00;
						to_camera_cmd[5]=0x02;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						break;
					case 0x93:				//自动增益
						to_camera_cmd[3]=0x04;
						to_camera_cmd[4]=0x00;
						to_camera_cmd[5]=0x02;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=0x02;
						to_camera_cmd[8]=0x00;
						to_camera_cmd[9]=0x00;
						to_camera_cmd[10]=0x00;
						break;
					case 0x94:				//手动曝光
						to_camera_cmd[3]=0x24;
						to_camera_cmd[4]=0x04;
						to_camera_cmd[5]=0x04;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						break;
					case 0x95:				//自动曝光
						to_camera_cmd[3]=0x24;
						to_camera_cmd[4]=0x04;
						to_camera_cmd[5]=0x04;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=0x02;
						to_camera_cmd[8]=0x00;
						to_camera_cmd[9]=0x00;
						to_camera_cmd[10]=0x00;
						break;
					case 0x96:				//Offest X
						to_camera_cmd[3]=0x44;
						to_camera_cmd[4]=0x02;
						to_camera_cmd[5]=0x03;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						break;
					case 0x97:				//Offest Y
						to_camera_cmd[3]=0x64;
						to_camera_cmd[4]=0x02;
						to_camera_cmd[5]=0x03;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=Recv_cmd_buf[3];
						to_camera_cmd[8]=Recv_cmd_buf[4];
						to_camera_cmd[9]=Recv_cmd_buf[5];
						to_camera_cmd[10]=Recv_cmd_buf[6];
						break;
					case 0x98:				//启动
						if((Recv_cmd_buf[3]==0x98)&&(Recv_cmd_buf[4]==0x98)&&(Recv_cmd_buf[4]==0x98)&&(Recv_cmd_buf[4]==0x98)){
							cmd=START_CAPTURE;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x99:				//停止
						if((Recv_cmd_buf[3]==0x99)&&(Recv_cmd_buf[4]==0x99)&&(Recv_cmd_buf[4]==0x99)&&(Recv_cmd_buf[4]==0x99)){
							cmd=STOP_CAPTURE;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x9A:				//传输启动
						if((Recv_cmd_buf[3]==0x9A)&&(Recv_cmd_buf[4]==0x9A)&&(Recv_cmd_buf[4]==0x9A)&&(Recv_cmd_buf[4]==0x9A)){
							cmd=TRANSFER;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x9B:				//停止传输
						if((Recv_cmd_buf[3]==0x9B)&&(Recv_cmd_buf[4]==0x9B)&&(Recv_cmd_buf[4]==0x9B)&&(Recv_cmd_buf[4]==0x9B)){
							cmd=STOP_TRANSFER;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x9C:				//关机
						if((Recv_cmd_buf[3]==0x9C)&&(Recv_cmd_buf[4]==0x9C)&&(Recv_cmd_buf[4]==0x9C)&&(Recv_cmd_buf[4]==0x9C)){
							cmd=POWER_OFF;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x9D:				//格式化硬盘
						if((Recv_cmd_buf[3]==0x9D)&&(Recv_cmd_buf[4]==0x9D)&&(Recv_cmd_buf[4]==0x9D)&&(Recv_cmd_buf[4]==0x9D)){
							cmd=MKFS_SDA;
							break;
						}else{
							cmd=-1;
							break;
						}
					case 0x9E:
						to_camera_cmd[3]=0x24;
						to_camera_cmd[4]=0x00;
						to_camera_cmd[5]=0x06;
						to_camera_cmd[6]=0x00;
						to_camera_cmd[7]=0x01;
						to_camera_cmd[8]=0x00;
						to_camera_cmd[9]=0x00;
						to_camera_cmd[10]=0x00;
						break;
					default:
						cout<<"default uart cmd"<<cmd<<endl;
						break;

					}
					return cmd;
				}
				return -1;
			}
		}
	}

}

int  uart_message_deal::deal_message(void){
	int cmd=-1;
	int ret;
	cmd=recv_from_cmd_uart();
	if(cmd>0x1F){			//cmd >0x1F才需要转发
		if(cmd==0x92){			//手动增益
			UART_Send(to_fpga_fd, (void *)set_camera_manual_gain, camera_cmd_size);
			usleep(10000);
			UART_Send(to_fpga_fd, (void *)to_camera_cmd, camera_cmd_size);
		}else if(cmd==0x94){		//手动曝光
			UART_Send(to_fpga_fd, (void *)set_camera_manual_exposure, camera_cmd_size);
			usleep(10000);
			UART_Send(to_fpga_fd, (void *)to_camera_cmd, camera_cmd_size);
		}else{
			UART_Send(to_fpga_fd, (void *)to_camera_cmd, camera_cmd_size);
		}
	}
	return cmd;
}



int uart_message_deal::fixed_time_return(unsigned int left_ssd_size,int now_state,unsigned int now_x,unsigned int now_y,unsigned int now_frame_rates){
	if(left_ssd_size>500 && left_ssd_size<0)
		return -1;
	Send_cmd_buf[2]=left_ssd_size & 0xFF;
	Send_cmd_buf[3]=(left_ssd_size>>8) & 0xFF;
	Send_cmd_buf[4]=now_state & 0xFF;
	Send_cmd_buf[5]=now_x & 0xFF;
	Send_cmd_buf[6]=(now_x>>8) & 0xFF;
	Send_cmd_buf[7]=now_y & 0xFF;
	Send_cmd_buf[8]=(now_y>>8) & 0xFF;
	Send_cmd_buf[9]=now_frame_rates & 0xFF;
	unsigned int sum = 0;
	for(int i = 0; i < (Send_total_size-2); i++){
		sum=sum + Send_cmd_buf[i];
//		printf("Send_cmd_buf [%d]= 0X%x \n",i,Send_cmd_buf[i]);
	}
//	printf("**************sum = 0X%x \n",sum);
	Send_cmd_buf[10]=sum&0xFF;
	Send_cmd_buf[11]=(sum>>8)&0xFF;
	int ret=UART_Send(cmd_uart_fd, (void *)Send_cmd_buf, Send_total_size);
	if(ret > 0){
		return ret;
	}else{
		cout<<"UART_Send Error\r\n"<<endl;
		return -2;
	}
}
