/*
 * main.cpp
 *
 *  Created on: 2016年12月11日
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



unsigned volatile char globalImageCacheState[MALLOC_NUM];  //状态标志 0 空闲     1 被采集占用    2 被存储占用
unsigned char* globalImageCache[MALLOC_NUM];
int csi_video_fd=0;//存储csi设备信息
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
	// 1.  查看本地/mnt/ 目录是不是存在
	if (access(CAPTURE_VIDEO_STORE_PATH,0))
	{
		printf("%s do not exist\n", CAPTURE_VIDEO_STORE_PATH);
		ret = mkdir(CAPTURE_VIDEO_STORE_PATH, 0777);  //如果不存在则手动创建目录
	}else{
		ret = 0;
		printf("%s exist\n", CAPTURE_VIDEO_STORE_PATH);
	}

		// 2. 查看 /dev/video0 节点是否存在 如果不存在说明驱动异常
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
 * 将保存全局图像缓冲区空闲状态的数组进行复位  进行一次新的采集任务
 * */
void globalImageCacheStateReset(){
	int i;
	for( i = 0; i < MALLOC_NUM; i++){
		if(globalImageCacheState[i] != 4){  // 4 是一个特殊的状态用于标记改位置的内存没有分配成功  TODO 可以使用枚举进行
			globalImageCacheState[i] = 0;
		}
	}
}

/**
 * 获取一个空闲的图像存储缓存区域
 * */
int getIdleImageCache(){
	int currentIndex = -1;
	for(int i = 0; i < MALLOC_NUM; i++){
//		printf("globalImageCacheState[%d] = %d \n",i,globalImageCacheState[i]);
		if( globalImageCacheState[(i + idleImageIndex) % MALLOC_NUM] == 0){
			globalImageCacheState[(i + idleImageIndex) % MALLOC_NUM] = 1;  // 1 标志已经被采集线程锁定了
			currentIndex = (i + idleImageIndex) % MALLOC_NUM;
			idleImageIndex = (idleImageIndex + 1) % MALLOC_NUM;  // TODO 需要调整是否需要记录之前的信息
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
//	idleImageIndex = (idleImageIndex + 1) % MALLOC_NUM;  // TODO 需要调整是否需要记录之前的信息
//	globalImageCacheState[currentIndex]=1;
//	return currentIndex;
//}

int getWritedImageCache(){
	int currentIndex = -1;
	for(int i = 0; i < MALLOC_NUM; i++){
		if( globalImageCacheState[(i + writeImageIndex) % MALLOC_NUM] == 2){  // 2 表示采集完成
			globalImageCacheState[(i + writeImageIndex) % MALLOC_NUM] = 3; //说明正在存储 标记现在被存储线程进行了锁定
			currentIndex = (i + writeImageIndex) % MALLOC_NUM;
			writeImageIndex = (writeImageIndex + 1) % MALLOC_NUM;  // TODO  如上
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
		if( globalImageCacheState[i] == 0){  // 0 表示空闲
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
//	while(wait_only_display_stop);				//显示未结束不允许采集
	startNewCapture = 1; //启动一个新的采集任务

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
//	while(wait_only_display_stop);				//显示未结束不允许采集
	startNewCapture = 1; //启动一个新的采集任务

}

#endif

/*
 * 执行停止采集操作 非阻塞
 * */
void doStopCaptureAction(){
	stopCapture = 1;
	waitPreCaputreAndSaveThreadOut = 1;

	printf("stopCapture=%d  waitPreCaputreAndSaveThreadOut=%d \n",stopCapture,waitPreCaputreAndSaveThreadOut);
	while(stopCapture){
		usleep(10);
	}  //等待当前采集和存储线程是否空闲
	while(waitPreCaputreAndSaveThreadOut){			//TODO
		usleep(10);
	}   // 当前队列中的数据已经全部保存完成

	currentSystemStatus = Idle;
}


int refresh_globalImageCache()						//为了做自适应，采集之前更新一下信息，血泪史。。。
{
	for(int i=0;i<DMA_MEM_NUM;i++){
		csi->abandon_some_frame(globalImageCache[0]);
	}
	csi->change_bits();			//获取真实图像的分辨率
	return 0;
}



/**
 * 释放所有的预分配的图片缓存空间
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


	//选定CSI大小
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
	led_lighten(light_no);					//初始化灯全拉低
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
	bits_fd= open(BITS_IO, O_RDWR);					//初始化与FPGA通信的管脚
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
	csi->get_resolution();			//获取真实图像的分辨率




	ret = init_local_store_device();
	if(ret != 0){
		goto exit_on_error;
	}
	display_buf=(unsigned char *)malloc(10*1024*1024);

	// 创建一个alive线程 用于提示程序是否在正常工作


#ifdef USE_UART
	uart_cotrol_p=new(uart_message_deal);

#endif

	ret = pthread_create(&tid_alive, NULL, aliveThread, (void *)NULL);
	if(ret != 0){
		printf("create alive thread error!\r\n");
		goto exit_on_error;
	}


	// 创建一个采集线程
	ret = pthread_create(&tid_capture, NULL, captureThread, (void *)NULL);
	if(ret != 0){
		printLog("create capture thread error!\r\n");
		goto exit_on_error;
	}
//
//	// 创建一个存储线程
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
					doStopCaptureAction();								//只有按下这个的时候才能显示
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
							demo_mode_with_display = 1;			//只有按下这个的时候才能显示
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
					demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
					usleep(100000);
					system("umount /dev/sda");
					usleep(1000);
					system("sync");
					usleep(1000);
					system("halt");
				}
				break;
			case MKFS_SDA:
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
				usleep(100000);
				printf("EXIT_ALL\n");
				goto exit_on_error;
			case REFRESH_HEAD:						//此特殊
				cout<<"REFRESH_HEAD"<<endl;
				demo_mode_with_display=0;
				usleep(10000);
				if(currentSystemStatus == Idle){
					refresh_globalImageCache();
				}
				break;
			case SET_BPP_8:
				cout<<"SET_BPP_8"<<endl;
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
				demo_mode_with_display=0;     //所有情况，有任务的时候就不能显示
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
exit_on_error:  // 通常程序不会跑到这里 如果运行到这里说明程序运行异常 需要重启系统  TODO 将所有的等点亮
	free_all();
	csi->stop_capture();
	delete(csi);
	delete(uart_cotrol_p);
	delete(hdmi_display_p);
	return 0;


}


