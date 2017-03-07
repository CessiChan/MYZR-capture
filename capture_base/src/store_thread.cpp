/*
 * store_thread.cpp
 *
 *  Created on: 2016年12月11日
 *      Author: user
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include<sys/sysinfo.h>
#include<sched.h>
#include<ctype.h>
#include<pthread.h>


#include "global.h"
#include "loss_test.h"
#include "store_thread.h"
#include "main.h"
#include "io_use.h"
#include "hdmi_display.h"
#include "store_struct.h"

//#define USE_FWRITE   //此方案下不能用这个
#define LOSS_TEST//******************************************************************
extern int volatile waitPreCaputreAndSaveThreadOut;
extern unsigned char volatile  globalImageCacheState[MALLOC_NUM];
extern unsigned char* globalImageCache[MALLOC_NUM];

static int file_fd = -1; // why here just for thread can close the file when out

#ifdef USE_FWRITE
FILE *fwrite_fd=NULL;
#endif

void use_num_to_file(char* currentStoreRootDirName, char* fileName,int num_temp){

	int len;
//	printf("\nBefore : %d    ",savefile_num);
	len = sprintf(fileName, "%s/%s/%08d", CAPTURE_VIDEO_STORE_PATH, currentStoreRootDirName, num_temp);
//	printf("After : %d    \n",savefile_num);
	fileName[len] = '\0';

}


void save_one_globalImageCache(int cacheImageIndex){
	char fileName[200];
	unsigned char* fileHeader;
	int len;
	int ret;
	memset(fileName, 0, 200);
	unsigned int file_num=*(unsigned int *)(globalImageCache[cacheImageIndex]+4);
//	printf("FILE-------%s,func------%s,LINE-------%d\n",__FILE__,__func__,__LINE__);
	use_num_to_file(currentStoreRootDirName,fileName,file_num);
//	printf("FILE-------%s,func------%s,LINE-------%d\n",__FILE__,__func__,__LINE__);
#ifdef real_time_display
	if(demo_mode_with_display==1){				//实时显示
		unsigned int position_temp=*(unsigned int *)(globalImageCache[cacheImageIndex]+32);
		real_X_SIZE_OF_FRAME=*(unsigned int *)(globalImageCache[cacheImageIndex]+12);
		real_Y_SIZE_OF_FRAME=*(unsigned int *)(globalImageCache[cacheImageIndex]+16);
//		real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME;
		for(int i=0;i<CSI_Frame_Counts;i++){
//			cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;//
			if((i%Display_per_frame)==0){
//				cout<<"real_X_SIZE_OF_FRAME ="<<real_X_SIZE_OF_FRAME<<" real_Y_SIZE_OF_FRAME ="<<real_Y_SIZE_OF_FRAME<<" real_lenth="<<real_lenth<<" real_FRAME_BPP="<<real_FRAME_BPP<<endl;
				if(real_FRAME_BPP==8)
					hdmi_display_p->lcd_write_all_bits_frame(real_X_SIZE_OF_FRAME,real_Y_SIZE_OF_FRAME,globalImageCache[cacheImageIndex]+real_lenth-position_temp+SIZE_OF_FPGA_HEAD+i*real_lenth,real_FRAME_BPP);
				else
					hdmi_display_p->lcd_write_all_bits_frame(real_X_SIZE_OF_FRAME,real_Y_SIZE_OF_FRAME,globalImageCache[cacheImageIndex]+real_lenth-position_temp*2+SIZE_OF_FPGA_HEAD+i*real_lenth,real_FRAME_BPP);
			}
		}
		return;
	}
	/*
	else{										//
		if((file_num%capture_display_per_csi)==0){
			unsigned int position_temp=*(unsigned int *)(globalImageCache[cacheImageIndex]+32);
//			real_X_SIZE_OF_FRAME=*(unsigned int *)(globalImageCache[cacheImageIndex]+12);
//			real_Y_SIZE_OF_FRAME=*(unsigned int *)(globalImageCache[cacheImageIndex]+16);
//			real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME;
			hdmi_display_p->lcd_write_8bits_frame(real_X_SIZE_OF_FRAME,real_Y_SIZE_OF_FRAME,globalImageCache[cacheImageIndex]+real_lenth-position_temp+SIZE_OF_FPGA_HEAD);
		}
	}
*/
#endif
#ifdef LOSS_TEST
//	cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
	if(file_num!=last_frame+1){
		cout<<"Lost frame :"<<""<<file_num<<",   Lost count = "<<file_num-last_frame-1<<endl;
		data_buf_count();				//TODO

	}

	last_frame=file_num;
#endif


//	cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
	if(isCapturing == 1){
		file_fd = open(fileName, O_RDWR | O_CREAT);
//		if(file_fd == -1){
//			printf("Create File %s Error\r\n", fileName);
//		}
		len = 0;
		fileHeader=globalImageCache[cacheImageIndex];
			while(len < SIZE_OF_CSI){
				ret = write(file_fd,fileHeader , SIZE_OF_CSI - len);
				if(ret > 0){
					len += ret;
					fileHeader += ret;
				}else{
					printf("Write Error\r\n");
					break;
				}
		}
		close(file_fd);
	}


}


void *storeFileThread(void *para_of_pic)
{

	cpu_set_t mask;  //CPU核的集合
	cpu_set_t get;	 //获取在集合中的CPU
	int a=3;		//运行在cpu2上
	CPU_ZERO(&mask);	//置空
	CPU_SET(a,&mask);	//设置亲和力值
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//设置线程CPU亲和力
	{
		printf("warning: could not set CPU affinity, continuing...\n");
	}
	int write_index = 0;
	while(1){
//		usleep(100);
//		continue;//休眠50毫秒
		// 1  从缓存数据队列中取到一个未保存的数据
		write_index = getWritedImageCache();
//		write_index=0;
//		cout<<"write_index ="<<write_index<<endl;
		if(write_index != -1){
			// 2 创建文件保存数据到文件中
			save_one_globalImageCache(write_index);
			globalImageCacheState[write_index] = 0;  //存储完成后修改成0 又可以被使用
			usleep(10);
			continue;
		}else if(waitPreCaputreAndSaveThreadOut != 1){			//waitPreCaputreAndSaveThreadOut为0
			if(is_all_idle()==MALLOC_NUM){
				usleep(100);
				continue;//休眠50毫秒
			}else{
				usleep(10);
				continue;//休眠50毫秒
			}


		}else{												//
			// out storefile thread
			if(is_all_idle()==MALLOC_NUM){
//				save_time_tool->end();
				waitPreCaputreAndSaveThreadOut = 0;
				usleep(100);
				continue;
			}else{
				usleep(10);
				continue;//休眠50毫秒
			}

		}
//		usleep(1000*50);//休眠50毫秒
	}
	return NULL;
}



