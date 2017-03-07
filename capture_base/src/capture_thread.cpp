/*
 * capture_thread.cpp
 *
 *  Created on: 2016��12��11��
 *      Author: user
 */

#ifndef CAPTURE_THREAD_CPP_
#define CAPTURE_THREAD_CPP_


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>


#include<sys/types.h>
#include<sys/sysinfo.h>
#include<unistd.h>
#include<sched.h>
#include<ctype.h>
#include<string.h>
#include<pthread.h>
#include "capture_thread.h"
#include "loss_test.h"
#include "global.h"
#include "store_struct.h"
#include "main.h"
#include "csi_module.h"
#include "io_use.h"

int volatile wait_only_display_stop = 0;
extern unsigned char* globalImageCache[MALLOC_NUM];
extern unsigned char volatile globalImageCacheState[MALLOC_NUM];
extern int startNewCapture;
//extern int isCapturing;
extern int stopCapture;
extern int volatile waitPreCaputreAndSaveThreadOut;
//extern volatile SystemStatus currentSystemStatus;



void *captureThread(void *ptr)
{

//	cpu_set_t mask;  //CPU�˵ļ���
//	cpu_set_t get;   //��ȡ�ڼ����е�CPU
//	int a=0;        //������cpu3 ��
//	CPU_ZERO(&mask);	//�ÿ�
//	CPU_SET(a,&mask);   //�����׺���ֵ
//	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//�����߳�CPU�׺���
//	{
//		printf("warning: could not set CPU affinity, continuing...\n");
//	}
	printf("In captureThread\n");
	int cacheImageIndex;
	int i=0;
//	struct video_frame_header *videoFrameHeader;
//	char fileNameTime[200];
//	int num_temp;//TODO
	while(1)
	{
#ifdef real_time_display
		if(demo_mode_with_display==1){
			for(i=0;i<DMA_MEM_NUM;i++){
				csi->abandon_some_frame(globalImageCache[0]);
			}
			is_real_time_display=1;
			while(demo_mode_with_display==1){
				while((cacheImageIndex = getIdleImageCache()) == -1){
					usleep(10);
				}
//				cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
				csi->capture_one_frame_from_csi(globalImageCache[cacheImageIndex]);
				globalImageCacheState[cacheImageIndex] = 2;//�洢����
			}
		}
		for(i=0;i<MALLOC_NUM;i++){
			globalImageCacheState[i] =0;
		}
		is_real_time_display=0;
#endif
		if(startNewCapture==1)  //��ʼһ���µĲɼ����� һ���µĲɼ�������Ҫ�����µĴ洢Ŀ¼
		{
			// 1 ֹͣ��ǰ�Ĳɼ����� �ȴ��洢������ɻ�����������е����ݵı���
			startNewCapture = 0;
			isCapturing = 1;
//			init_recount_states();
			if(stopCapture == 0 ){  //must be true

				for(i=0;i<DMA_MEM_NUM;i++){
					csi->abandon_some_frame(globalImageCache[0]);
				}

//				csi_time_tool->begin();

//					cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
				while(stopCapture == 0){
					while((cacheImageIndex = getIdleImageCache()) == -1){
						usleep(5);
					}
					csi->capture_one_frame_from_csi(globalImageCache[cacheImageIndex]);
					globalImageCacheState[cacheImageIndex] = 2;//�洢����
				}

//				csi_time_tool->end();
			}
				waitPreCaputreAndSaveThreadOut = 1;
				stopCapture = 0;
				isCapturing = 0;

			}else{
				usleep(1000*100);//����100����
				stopCapture = 0;
				isCapturing = 0;
			}


		}
return NULL;
}




#endif /* CAPTURE_THREAD_CPP_ */
