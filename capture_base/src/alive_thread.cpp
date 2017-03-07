/*
 * alive_thread.cpp
 *
 *  Created on: 2017��1��4��
 *      Author: user
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <iostream>
#include "global.h"
#include "alive_thread.h"
#include "store_struct.h"
#include "main.h"
#include "io_use.h"
#include "oled/oled.h"
#include "oled/management.h"
#include "command_function/uart_module/uart_control.h"

using namespace std;

extern volatile int currentSystemStatus;


//unsigned int getCapality() {
// char use_percent;
// unsigned int ssd_free_size;
//    FILE *pp = popen("df -h|grep /dev/sda", "r"); //�����ܵ�
//    if (!pp) {
//        return -1;
//    }
//    char tmp[100]; //����һ�����ʵĳ��ȣ��Դ洢ÿһ�����
//   fgets(tmp, sizeof(tmp), pp);
//
////		printf("51 %d\n",tmp[51]);
////		printf("52 %d\n",tmp[52]);
////		printf("53 %d\n",tmp[53]);
//
//		if(tmp[51]<48)
//			tmp[51]=48;
//		if(tmp[52]<48)
//			tmp[52]=48;
//
//    pclose(pp); //�رչܵ�
//	use_percent=100*(tmp[51]-48)+10*(tmp[52]-48)+(tmp[53]-48);
//	ssd_free_size=(100-use_percent)*SSD_SIZE;
// return ssd_free_size;
//}



int pown_down_fd;
int believe__pown_down=0;

#ifdef USE_OLED
int init_my_spi(void){
	myspi_fd = open(SPI_NAME, O_RDWR);
	if(myspi_fd < 0){
		printf("open %s fail.\n", SPI_NAME);
		return -1;
	}

	OLED_Init();	//��ʼ��OLED
	OLED_Clear();
	oled_set();
	return 0;
}
#endif

int read_pown_down(){				//�ػ��ж�
	char read_data;
	int bytes;


	bytes=read(pown_down_fd, &read_data, sizeof(char));
	 if(bytes==-1)
    {
		 return -1;
//        printf("ReadFailed.\n");
    }
    else
    {
        if(read_data==0){
        	believe__pown_down++;
        	if(believe__pown_down==2){
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
				usleep(100000);
        		cout<<"^^^^^^^^^^^^^^Pown^^^^^^^^^^^^^^^^^^^Down^^^^^^^^^^^^^^\n"<<endl;
        		system("sync");
        		system("umount /dev/sda");
        		system("halt");   		//�ػ�
        		system("poweroff");
        	}
        }
    }
return 0;

}

unsigned int getCapality()
{
	struct statfs diskInfo;
	if(statfs(STORAGE_POSITION, &diskInfo)!=0)
	{
		printLog("reading storage information failed,trying to mount again\n");
		if(statfs(STORAGE_POSITION, &diskInfo)!=0)
		printLog("mount failed please communicated the produtor\n");
	}
	unsigned long long totalBlocks = diskInfo.f_bsize;
	unsigned long long totalSize = totalBlocks * diskInfo.f_blocks;
	size_t mbTotalsize = totalSize>>20;
	unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;
	size_t mbFreedisk = freeDisk>>20;
//	printLog ("%s  total=%dMB, free=%dMB\n", STORAGE_POSITION,mbTotalsize, mbFreedisk);
	return mbFreedisk >> 10;
}

void *aliveThread(void *ptr){
	cpu_set_t mask;  //CPU�˵ļ���
	cpu_set_t get;	 //��ȡ�ڼ����е�CPU
	int a=3;		//������cpu0��
	CPU_ZERO(&mask);	//�ÿ�
	CPU_SET(a,&mask);	//�����׺���ֵ

	unsigned int left_ssd_size;

	init_my_spi();
	pown_down_fd = open(POWER_DOWN, O_RDWR);
	if(pown_down_fd < 0){
		printf("open %s fail.\n", POWER_DOWN);
	}

	while(1){
#ifdef USE_UART
		left_ssd_size = getCapality()-23;
//		left_ssd_size=430;
		if(left_ssd_size<3){
			if(currentSystemStatus == Capturing){
					//isNeedOpenDisplayWhenCapturing = 0;
					doStopCaptureAction();
				}
		}
//		cout <<" left_ssd_size is "<<left_ssd_size<<endl;
		uart_cotrol_p->fixed_time_return(left_ssd_size,currentSystemStatus,real_X_SIZE_OF_FRAME,real_Y_SIZE_OF_FRAME,Frame_rate);
		usleep(80*1000);
#endif
#ifdef USE_OLED
		left_ssd_size = getCapality();				//ʵ�ʴ�С�ȵó�����С
//		left_ssd_size=430;
		if(left_ssd_size<2){
			if(currentSystemStatus == Capturing){
					//isNeedOpenDisplayWhenCapturing = 0;
					doStopCaptureAction();
					cout<<"SSD is full!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
				}
		}

		read_pown_down();
//		cout <<" left_ssd_size is "<<left_ssd_size<<endl;
//		uart_cotrol_p->fixed_time_return(left_ssd_size,currentSystemStatus,real_X_SIZE_OF_FRAME,real_Y_SIZE_OF_FRAME,Frame_rate);
		ShowManagement(left_ssd_size,real_FRAME_BPP);		//oled���³���
		usleep(700000);				//0.7s����һ��

#endif
	}
	return NULL;
}



void *launch_thread(void *ptr)
{
//	cpu_set_t mask;  //CPU�˵ļ���
//	cpu_set_t get;	 //��ȡ�ڼ����е�CPU
//	int a=0;		//������cpu0��
//	CPU_ZERO(&mask);	//�ÿ�
//	CPU_SET(a,&mask);	//�����׺���ֵ

//		system("date");
		currentSystemStatus = Transfering;
		system("java -jar /capture/BoardService.jar");
		//system("umount /dev/sda");


	return NULL;
}
