/*
 * main.cpp
 *
 *  Created on: 2016��12��11��
 *      Author: user
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <fstream>

#include "loss_test.h"
#include "capture_thread.h"
#include "global.h"
#include "store_struct.h"
#include "csi_module.h"
#include "PropertyManager.h"
#include "main.h"
#include "store_thread.h"
#include "io_use.h"
#include "hdmi_display.h"
#include "alive_thread.h"
#include "back_display.h"
#include "command_function/uart_module/uart_control.h"
#include "command_function/command.h"
#include "command_function/net_module/net_thread.h"
#include "command_function/cmd_thread.h"
#include "command_function/CommandManager.h"
using namespace std;
using namespace wj;


int volatile startNewCapture = 0;
int volatile isCapturing = 0;
int volatile stopCapture = 0;
int volatile waitPreCaputreAndSaveThreadOut = 0;
//volatile SystemStatus currentSystemStatus = Idle;
volatile int currentSystemStatus = Idle;
volatile int stop_flag_test=1;



unsigned volatile char globalImageCacheState[MALLOC_NUM];  //״̬��־ 0 ����     1 ���ɼ�ռ��    2 ���洢ռ��
unsigned char* globalImageCache[MALLOC_NUM];
int csi_video_fd=0;//�洢csi�豸��Ϣ
char currentStoreRootDirName[60];

static int idleImageIndex = 0;
static int writeImageIndex = 0;

int init_local_store_device(){
	int i;
	int ret;
	for( i = 0; i < MALLOC_NUM; i++){
		globalImageCache[i] = (unsigned char *)malloc(SIZE_OF_CSI+20);
		globalImageCacheState[i] = 0;
		if(globalImageCache[i] == NULL){
			printf("____________malloc error:i = %d \r\n", i);
			globalImageCacheState[i] = 4;  // here do something
		}

	}
	printf("---------------------init_local_store_device--------------\n");
	printf("MALLOC_NUM   	  =%d \n",MALLOC_NUM);
	printf("SIZE_OF_CSI =%d \n",SIZE_OF_CSI);
	printf("----------------------------------------------------------\n");
	// 1.  �鿴����/mnt/ Ŀ¼�ǲ��Ǵ���
	if (access(CAPTURE_VIDEO_STORE_PATH,0))
	{
		printf("%s do not exist\n", CAPTURE_VIDEO_STORE_PATH);
		ret = mkdir(CAPTURE_VIDEO_STORE_PATH, 0777);  //������������ֶ�����Ŀ¼
	}else{
		ret = 0;
		printf("%s exist\n", CAPTURE_VIDEO_STORE_PATH);
	}

		// 2. �鿴 /dev/video0 �ڵ��Ƿ���� ���������˵�������쳣
		if (access(CAMERA_DEVICE, 0)){
			printf("%s do not exist driver error \n", CAMERA_DEVICE);
			return -1;
		}else{
			ret = 0;
			printf("%s exist\n", CAMERA_DEVICE);
		}




	return 0;
}
/**
 * ������ȫ��ͼ�񻺳�������״̬��������и�λ  ����һ���µĲɼ�����
 * */
void globalImageCacheStateReset(){
	int i;
	for( i = 0; i < MALLOC_NUM; i++){
		if(globalImageCacheState[i] != 4){  // 4 ��һ�������״̬���ڱ�Ǹ�λ�õ��ڴ�û�з���ɹ�  TODO ����ʹ��ö�ٽ���
			globalImageCacheState[i] = 0;
		}
	}
}

/**
 * ��ȡһ�����е�ͼ��洢��������
 * */
int getIdleImageCache(){
	int currentIndex = -1;
	for(int i = 0; i < MALLOC_NUM; i++){
//		printf("globalImageCacheState[%d] = %d \n",i,globalImageCacheState[i]);
		if( globalImageCacheState[(i + idleImageIndex) % MALLOC_NUM] == 0){
			globalImageCacheState[(i + idleImageIndex) % MALLOC_NUM] = 1;  // 1 ��־�Ѿ����ɼ��߳�������
			currentIndex = (i + idleImageIndex) % MALLOC_NUM;
			idleImageIndex = (idleImageIndex + 1) % MALLOC_NUM;  // TODO ��Ҫ�����Ƿ���Ҫ��¼֮ǰ����Ϣ
//			idleImageIndex = 0;
//			printf("getIdleImageCache: globalImageCacheState[%d]==%d \n",currentIndex,globalImageCacheState[currentIndex]);
			return currentIndex;
		}

	}
//	printf("current no idle image cache\r\n");
	return currentIndex;
}

//int getIdleImageCache(){
//	int currentIndex = -1;
//	currentIndex=idleImageIndex % MALLOC_NUM;
//	idleImageIndex = (idleImageIndex + 1) % MALLOC_NUM;  // TODO ��Ҫ�����Ƿ���Ҫ��¼֮ǰ����Ϣ
//	globalImageCacheState[currentIndex]=1;
//	return currentIndex;
//}

int getWritedImageCache(){
	int currentIndex = -1;
	for(int i = 0; i < MALLOC_NUM; i++){
		if( globalImageCacheState[(i + writeImageIndex) % MALLOC_NUM] == 2){  // 2 ��ʾ�ɼ����
			globalImageCacheState[(i + writeImageIndex) % MALLOC_NUM] = 3; //˵�����ڴ洢 ������ڱ��洢�߳̽���������
			currentIndex = (i + writeImageIndex) % MALLOC_NUM;
			writeImageIndex = (writeImageIndex + 1) % MALLOC_NUM;  // TODO  ����
//			writeImageIndex = 0;
//			printf("getWritedImageCache: globalImageCacheState[%d]==%d \n",currentIndex,globalImageCacheState[currentIndex]);
			return currentIndex;
		}
	}
//	printf("current no can write image cache\r\n");
	return currentIndex;
}


int is_all_idle(){
	int currentIndex=0;
	for(int i = 0; i < MALLOC_NUM; i++){
		if( globalImageCacheState[i] == 0){  // 0 ��ʾ����
			 currentIndex++;
		}
	}
//	printf("current no can write image cache\r\n");
	return currentIndex;


}

int data_buf_count(){
	printf("\n");
	for(int i = 0; i < MALLOC_NUM; i++){
		printf("State [%d] =%d,",i,globalImageCacheState[i]);
	}
	printf("\n");

return 0;
}



#ifdef USE_UART
void doCaptureAction(){
	int rootDirLen;
	int len;
	char rootDir[100];
	int ret;

	currentSystemStatus = Capturing;
	rootDirLen = sprintf(currentStoreRootDirName, "%02d-%02d-%02d-%02d-%02d-%02d-%02d", uart_cotrol_p->Recv_cmd_buf[13], uart_cotrol_p->Recv_cmd_buf[14], uart_cotrol_p->Recv_cmd_buf[15],
			uart_cotrol_p->Recv_cmd_buf[16],uart_cotrol_p->Recv_cmd_buf[17], uart_cotrol_p->Recv_cmd_buf[18], uart_cotrol_p->Recv_cmd_buf[19]);
	currentStoreRootDirName[rootDirLen] = '\0';
	memset(rootDir, 0, 100);
	len = sprintf(rootDir, "%s/%s","/mnt", currentStoreRootDirName);
	rootDir[len] = '\0';
		// create a new dir
	cout << "Create new SaveDir:  "<<rootDir<<endl;
		ret = mkdir(rootDir, 0755);
//		PropertyManager::updateLastCaptureLocation(rootDir);
//		printf("currentStoreRootDirName = %s \r\n", currentStoreRootDirName);
//	while(wait_only_display_stop);				//��ʾδ����������ɼ�
	startNewCapture = 1; //����һ���µĲɼ�����

}
#else
void doCaptureAction(){
	struct tm* currentTime;
	struct timeval current;
	int rootDirLen;
	int len;
	char rootDir[200];
	int ret;

	currentSystemStatus = Capturing;
	gettimeofday(&current, NULL);
	currentTime = gmtime(&current.tv_sec);
	rootDirLen = sprintf(currentStoreRootDirName, "%02d-%02d-%02d-%02d-%02d-%02d-%02d",currentTime->tm_year-100, 1 + currentTime->tm_mon, currentTime->tm_mday,
				currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec, (int)current.tv_usec%1000000/10000);
	currentStoreRootDirName[rootDirLen] = '\0';
		memset(rootDir, 0, 200);
		len = sprintf(rootDir, "%s/%s", CAPTURE_VIDEO_STORE_PATH, currentStoreRootDirName);
		rootDir[len] = '\0';
		// create a new dir
		printf("Create new SaveDir: %s \r\n", rootDir);
		ret = mkdir(rootDir, 0755);
//		PropertyManager::updateLastCaptureLocation(rootDir);
		printf("currentStoreRootDirName = %s \r\n", currentStoreRootDirName);
//	while(wait_only_display_stop);				//��ʾδ����������ɼ�
	startNewCapture = 1; //����һ���µĲɼ�����

}

#endif

/*
 * ִ��ֹͣ�ɼ����� ������
 * */
void doStopCaptureAction(){
	stopCapture = 1;
	waitPreCaputreAndSaveThreadOut = 1;

	printf("stopCapture=%d  waitPreCaputreAndSaveThreadOut=%d \n",stopCapture,waitPreCaputreAndSaveThreadOut);
	while(stopCapture){
		usleep(10);
	}  //�ȴ���ǰ�ɼ��ʹ洢�߳��Ƿ����
	while(waitPreCaputreAndSaveThreadOut){			//TODO
		usleep(10);
	}   // ��ǰ�����е������Ѿ�ȫ���������

	currentSystemStatus = Idle;
}


int refresh_globalImageCache()						//Ϊ��������Ӧ���ɼ�֮ǰ����һ����Ϣ��Ѫ��ʷ������
{
	for(int i=0;i<DMA_MEM_NUM;i++){
		csi->abandon_some_frame(globalImageCache[0]);
	}
	csi->change_bits();			//��ȡ��ʵͼ��ķֱ���
	return 0;
}



/**
 * �ͷ����е�Ԥ�����ͼƬ����ռ�
 * */
void free_all()
{
	int i;
	for(i=0;i<MALLOC_NUM;i++)
		free(globalImageCache[i]);
}
void printMenu(){
	printf(".........Menu.......\r\n");
	printf("%d, Start Capture\r\n", START_CAPTURE);
	printf("%d, Stop Capture\r\n", STOP_CAPTURE);
	printf("%d, HDMI_DISPLAY\r\n", HDMI_DISPLAY);
	printf("%d, STOP_DISPLAY\r\n", STOP_DISPLAY);
	printf("%d, TRANSFER \r\n", TRANSFER);
	printf("%d, STOP_TRANSFER\r\n", STOP_TRANSFER);
	printf("%d, POWER_OFF\r\n", POWER_OFF);
	printf("%d, MKFS_SDA\r\n", MKFS_SDA);
	printf("%d, EXIT_ALL\r\n", EXIT_ALL);
	printf("%d, REFRESH_HEAD\n",REFRESH_HEAD);
	printf("*******************\r\n");
}


int main() {			//chengsi
	int ret;
	int cmd=0;
	int i;
	printf("\ncsi_frame_head'size = %d\n",sizeof(csi_frame_head));
	system("rm -rf /capture/hs_err_pid1*");
	system("rm -rf /hs_err_pid1*");


	//ѡ��CSI��С
	cout<<"Please Input SEND_X  and  SEND_Y"<<endl;
//	while(cin>>SEND_X>>SEND_Y);
	cin>>SEND_X>>SEND_Y;
	if(SEND_X==1080){
		system("sh /capture/csi_choose.sh 1080");
	}else if(SEND_X==1920){
		system("sh /capture/csi_choose.sh 1920");
	}else if(SEND_X==3000){
		system("sh /capture/csi_choose.sh 3000");
	}else if(SEND_X==3360){
		system("sh /capture/csi_choose.sh 3360");
	}else if(SEND_X==3840){
		system("sh /capture/csi_choose.sh 3840");
	}

	SIZE_OF_CSI=SEND_X*SEND_Y;




#ifdef USE_LED
	led_open();
	char light_no=5;
	led_lighten(light_no);					//��ʼ����ȫ����
#endif

	int bits_fd;
	char bits_no;

	pthread_t tid_capture;
	pthread_t tid_save;
	pthread_t tid_alive;
	pthread_t tid_launch;
//	double speed=0;
	printf("Hello world!\n");
#ifdef USE_IO_TEST
	led_fd = open(DEVICE_NAME, O_RDWR);
	if(led_fd < 0){
		printf("open %s fail.\n", DEVICE_NAME);
	}
#endif
	hdmi_display_p=new(hdmi_display);


#ifdef USE_BITS_IO
	bits_fd= open(BITS_IO, O_RDWR);					//��ʼ����FPGAͨ�ŵĹܽ�
	if(bits_fd < 0){
		printf("open %s fail.\n", BITS_IO);
	}
//	bits_no=char(real_FRAME_BPP);
	bits_no=14;
	write(bits_fd, &bits_no,1);
	sleep(3);
#endif



//	csi_time_tool=new(TimeTool);
//	save_time_tool=new(TimeTool);
//	memcpy_time_tool=new(TimeTool);
	csi = new Csi();
	if(csi->init_csi(SEND_X/3, SEND_Y, SEND_X/3, SEND_Y, 0, 0, 60) < 0){
		printf("init_csi fail.\n");
		goto exit_on_error;
	}
	if(csi->start_capture(DMA_MEM_NUM) < 0){
		printf("start capture fail!\n");
		return -1;
	}
	printf("init csi complete!\r\n");
	csi->get_resolution();			//��ȡ��ʵͼ��ķֱ���




	ret = init_local_store_device();
	if(ret != 0){
		goto exit_on_error;
	}
	display_buf=(unsigned char *)malloc(10*1024*1024);

	// ����һ��alive�߳� ������ʾ�����Ƿ�����������


#ifdef USE_UART
	uart_cotrol_p=new(uart_message_deal);

#endif

	ret = pthread_create(&tid_alive, NULL, aliveThread, (void *)NULL);
	if(ret != 0){
		printf("create alive thread error!\r\n");
		goto exit_on_error;
	}


	// ����һ���ɼ��߳�
	ret = pthread_create(&tid_capture, NULL, captureThread, (void *)NULL);
	if(ret != 0){
		printLog("create capture thread error!\r\n");
		goto exit_on_error;
	}
//
//	// ����һ���洢�߳�
	ret = pthread_create(&tid_save, NULL, storeFileThread, (void *)NULL);
	if(ret != 0){
		printLog("create save thread error!\r\n");
		goto exit_on_error;
	}





#ifdef USE_UART
	pthread_t uart_cmd_tid;
		ret = pthread_create(&uart_cmd_tid, NULL, uart_cmd_thread, NULL);
#endif
#ifdef USE_NET
		pthread_t net_cmd_tid;
		ret = pthread_create(&net_cmd_tid, NULL, command_receiver_thread, NULL);
#endif
#ifdef USE_CRT
		pthread_t crt_cmd_tid;
		ret = pthread_create(&crt_cmd_tid, NULL, crt_cmd_thread, NULL);
#endif

#ifdef USE_KEY
		pthread_t key_cmd_tid;
		ret = pthread_create(&key_cmd_tid, NULL, key_cmd_thread, NULL);
#endif

		CommandManager *commandManager;
		commandManager = CommandManager::getInstance();

	while(1)
	{


		cmd = commandManager->getNextCommandBlock();
#ifdef PRINT_SOMETHING
		if(cmd<100){
			printMenu();
		}
		if(cmd<100){
			printf("read command : %d\n",cmd);
			}
//		cout << "read command : " << cmd <<  endl;
/*
		if(cmd == 255){  // heart package
			cout << heartCount << " heart package" << endl;
			heartCount++;
		}
		*/
#endif
//		printf("current SystemStatu: %d \r\n", currentSystemStatus);
		switch(cmd)
		{
			case START_CAPTURE:
				cout<<"StartCapture\r\n"<<endl;
				demo_mode_with_display=0;
			if (currentSystemStatus == Displaying) {
					stopDisplayVideo();		
				}
				if(currentSystemStatus == Transfering){
					usleep(170*1000);
					system("ps -ef|grep BoardService|grep -v grep|awk '{print $1}'|xargs kill -12");
					currentSystemStatus = Idle;
				}
				if(currentSystemStatus == Idle){
//					cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
					usleep(10);
					doCaptureAction();
#ifdef USE_LED
						light_no=1;
						led_lighten(light_no);
#endif
				}
				break;
			case STOP_CAPTURE:
				printf("StopCapture\r\n");
				if (currentSystemStatus == Displaying) {
					stopDisplayVideo();		
				}
				if(currentSystemStatus == Transfering){
					usleep(170*1000);
					system("ps -ef|grep BoardService|grep -v grep|awk '{print $1}'|xargs kill -12");
					currentSystemStatus = Idle;
				}
				if(currentSystemStatus == Capturing){
//					csi_time_tool->end();
					doStopCaptureAction();								//ֻ�а��������ʱ�������ʾ
					demo_mode_with_display=1;
#ifdef USE_LED
						light_no=5;
						led_lighten(light_no);
#endif
				}
				break;
			case HDMI_DISPLAY:
				if(currentSystemStatus == Transfering){
					system("ps -ef|grep BoardService|grep -v grep|awk '{print $1}'|xargs kill -12");
					currentSystemStatus = Idle;
					sleep(1);
				}
				if(currentSystemStatus == Capturing){
					doStopCaptureAction();	
					currentSystemStatus = Idle;
					sleep(1);
				}
				if(currentSystemStatus == Idle){
					printf("HDMI_DISPLAY\r\n");
					demo_mode_with_display=0;
					usleep(100);
#ifdef USE_LED
						light_no=3;
						led_lighten(light_no);
#endif
					startDisplayVideo();
				}else{
					// do nothing
				}
				break;
			case STOP_DISPLAY:
			if (currentSystemStatus == Displaying) {
				while (1) {
					stopDisplayVideo();
						
					if(is_doing_display==0){
						if (currentSystemStatus == Idle) {
							demo_mode_with_display = 1;			//ֻ�а��������ʱ�������ʾ
#ifdef USE_LED
						light_no=5;
						led_lighten(light_no);
#endif
							break;
						}
					}
					usleep(1000);
				// current system is idle cannot do stop action
				}
			}
				break;
			
			case TRANSFER:
				printf("TRANSFER\n");
				if (currentSystemStatus == Displaying) {
					stopDisplayVideo();		
				}
				if(currentSystemStatus == Capturing){
					doStopCaptureAction();
				}
				if(currentSystemStatus == Transfering){
					break;
				}

					if(currentSystemStatus == Idle){
						demo_mode_with_display=0;
						sleep(1);
						ret = pthread_create(&tid_launch, NULL, launch_thread, (void *)NULL);
						if(ret != 0){
							printf("create launch thread error!\r\n");
							goto exit_on_error;
						}
						currentSystemStatus = Transfering;
#ifdef USE_LED
						light_no=2;
						led_lighten(light_no);
#endif
					}

				break;
			case STOP_TRANSFER:
				cout<<"Stop transfer "<<endl;
				if (currentSystemStatus == Displaying) {
					stopDisplayVideo();		
				}
				if(currentSystemStatus == Transfering){
					system("ps -ef|grep BoardService|grep -v grep|awk '{print $1}'|xargs kill -12");
					currentSystemStatus = Idle;
				}
				sleep(1);
				demo_mode_with_display=1;
				currentSystemStatus = Idle;
#ifdef USE_LED
						light_no=5;
						led_lighten(light_no);
#endif
				break;
			case POWER_OFF:
				if(currentSystemStatus == Idle){
					demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
					usleep(100000);
					system("umount /dev/sda");
					usleep(1000);
					system("sync");
					usleep(1000);
					system("halt");
				}
				break;
			case MKFS_SDA:
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
				sleep(1);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						system("umount /dev/sda");
						usleep(1000);
						system("mkfs.ext2 /dev/sda");
						sleep(3);
						system("halt");
						break;
					}
				}
				break;
			case EXIT_ALL:
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
				usleep(100000);
				printf("EXIT_ALL\n");
				goto exit_on_error;
			case REFRESH_HEAD:						//������
				cout<<"REFRESH_HEAD"<<endl;
				demo_mode_with_display=0;
				usleep(10000);
				if(currentSystemStatus == Idle){
					refresh_globalImageCache();
				}
				break;
			case SET_BPP_8:
				cout<<"SET_BPP_8"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
						light_no=6;
						led_lighten(light_no);
#endif
				usleep(100000);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						real_FRAME_BPP=8;
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						printf("now BPP is %d \n",real_FRAME_BPP);
#ifdef USE_LED
						light_no=7;
						led_lighten(light_no);
#endif
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
				demo_mode_with_display=1;
				break;
			case SET_BPP_10:
				cout<<"SET_BPP_10"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
						light_no=6;
						led_lighten(light_no);
#endif
				usleep(100000);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						real_FRAME_BPP=10;
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						printf("now BPP is %d \n",real_FRAME_BPP);
#ifdef USE_LED
						light_no=7;
						led_lighten(light_no);
#endif
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
				demo_mode_with_display=1;
				break;
			case SET_BPP_12:
				cout<<"SET_BPP_12"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
						light_no=6;
						led_lighten(light_no);
#endif
				usleep(100000);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						real_FRAME_BPP=12;
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						printf("now BPP is %d \n",real_FRAME_BPP);
#ifdef USE_LED
						light_no=7;
						led_lighten(light_no);
#endif
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
				demo_mode_with_display=1;
				break;
			case SET_BPP_14:
				cout<<"SET_BPP_14"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
						light_no=6;
						led_lighten(light_no);
#endif
				usleep(100000);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						real_FRAME_BPP=14;
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						printf("now BPP is %d \n",real_FRAME_BPP);
#ifdef USE_LED
						light_no=7;
						led_lighten(light_no);
#endif
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
				demo_mode_with_display=1;
				break;
			case SET_BPP_16:
				cout<<"SET_BPP_16"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
						light_no=6;
						led_lighten(light_no);
#endif
				usleep(100000);
				while(1){
					if((currentSystemStatus ==Idle)&&(is_real_time_display==0)){
						real_FRAME_BPP=16;
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						printf("now BPP is %d \n",real_FRAME_BPP);
#ifdef USE_LED
						light_no=7;
						led_lighten(light_no);
#endif
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
				demo_mode_with_display=1;
				break;
			case BOTTON_ADD_BITS:
				cout<<"BOTTON_ADD_BITS"<<endl;
				demo_mode_with_display=0;     //����������������ʱ��Ͳ�����ʾ
#ifdef USE_LED
					light_no=6;
					led_lighten(light_no);
#endif
				usleep(70000);
				while(1){
					if((currentSystemStatus ==Idle)||(is_real_time_display==0)){
						real_FRAME_BPP=real_FRAME_BPP+2;
						if(real_FRAME_BPP>16){
							real_FRAME_BPP=8;
						}
						bits_no=char(real_FRAME_BPP);
						write(bits_fd, &bits_no,1);
						sleep(1);
						refresh_globalImageCache();
						usleep(1000);
						break;
					}else{
						usleep(1000);
					}
				}
#ifdef USE_LED
					light_no=7;
					led_lighten(light_no);
#endif

				break;
			default :
//				printf("error cmd\r\n");
				break;
		}
	}

	pthread_join(tid_save, NULL);
	pthread_join(tid_capture, NULL);
	pthread_join(tid_alive, NULL);
#ifdef	USE_UART
	pthread_join(uart_cmd_tid, NULL);
#endif
#ifdef	USE_NET
	pthread_join(net_cmd_tid, NULL);
#endif
#ifdef	USE_CRT
	pthread_join(crt_cmd_tid, NULL);
#endif
exit_on_error:  // ͨ�����򲻻��ܵ����� ������е�����˵�����������쳣 ��Ҫ����ϵͳ  TODO �����еĵȵ���
	free_all();
	csi->stop_capture();
	delete(csi);
	delete(uart_cotrol_p);
	delete(hdmi_display_p);
	return 0;


}


