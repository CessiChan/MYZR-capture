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


unsigned char *display_buf=NULL;
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
	if((demo_mode_with_display==1)&&(isCapturing==0)){				//实时显示
		global_cache_display(globalImageCache[cacheImageIndex]);
		return;
	}
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



int global_cache_display(unsigned char *file_head){
		unsigned int left_lenth=0;
//		int sleeptime;
		unsigned int csi_frame_count_temp=*(unsigned int *)(file_head+4);
		unsigned int frame_rates_temp=*(unsigned int *)(file_head+8);
		int xSize_temp=*(int *)(file_head+12);
		int ySize_temp=*(int *)(file_head+16);
		unsigned int bpp_temp=*(unsigned int *)(file_head+20);;
//		unsigned int max_temp=p_vide_frame_info->max;
//		unsigned int min_temp=p_vide_frame_info->min;
		unsigned int position_temp=*(unsigned int *)(file_head+32);
		unsigned int DATA_SIZE_temp=SIZE_OF_CSI-SIZE_OF_FPGA_HEAD;
		unsigned char *videoFramePicStart=file_head+36;
		unsigned int real_lenth_temp;
		if(bpp_temp==8){
			real_lenth_temp=xSize_temp*ySize_temp;
		}else{
			real_lenth_temp=xSize_temp*ySize_temp*2;
		}
		static int i_cs=0;				//每次调用值都保存
//		printf("csi_frame_count_temp =%d ,frame_rates_temp =%d ,position_temp =%d\n",csi_frame_count_temp,frame_rates_temp, position_temp);
		if(last_csi_count>0){								//判断是否丢帧,last_csi_count初始化为0的，因此第一次不会误进入此处
			if(csi_frame_count_temp==(last_csi_count+1)&&(now_get_frame_size==position_temp)){			//未丢帧
				last_csi_count=csi_frame_count_temp;				//记录信息
//				cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
			}else{											//丢帧
					init_recount_states();							//放弃一切，重新开始。。。
					cout<<"Display LOSS FRAME"<<endl;
				}

		}
		/*************************************************处理数据*****************************************
		*int now_block_count=-1;						//用于多头，已经取得第几个block，从0开始
		*unsigned int now_get_frame_size=0;			//取得的数据量
		*unsigned int now_get_csi_size=0;				//csi中已经解析的数据，最大值为DATA_SIZE
		*unsigned int last_csi_count=0;				//上一次取得的CSI的计数
		**************************************************************************************************/

			while (1) {
//				cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
				if ((now_get_frame_size == 0)&&(last_csi_count==0)&&(now_get_csi_size==0)) {					//第一帧的处理方式
					if(position_temp==0){
						left_lenth=0;
					}else{
						if(bpp_temp==8)					//8bits
							left_lenth=real_lenth - position_temp;
						else							//bigger than 8bits
							left_lenth=real_lenth - position_temp*2;
					}

					if (left_lenth > DATA_SIZE_temp) {
						break;					//分辨率太大，第一帧内找不到
					} else {
						last_csi_count = csi_frame_count_temp;
						if ((real_lenth_temp <= (DATA_SIZE_temp - left_lenth))&& ((DATA_SIZE_temp - left_lenth) > 0)) {//内有一帧完整数据
//							memcpy((void *) (display_buf_temp),videoFramePicStart + left_lenth, real_lenth_temp);
//							cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<"i_cs-------"<<i_cs<<"  ,CSI_Frame_Counts"<<CSI_Frame_Counts<<endl;
							if((i_cs%Display_per_frame)==0){
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
								hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,videoFramePicStart + left_lenth,bpp_temp);//閺勫墽銇氱�癸拷
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
							}
							i_cs=(i_cs++)%40320;
							now_get_csi_size = real_lenth_temp + left_lenth;
							now_get_frame_size = 0;		//记录信息
							if (now_get_csi_size == DATA_SIZE_temp) {				//数据刚好取完了
								now_get_csi_size = 0;
								break;
							}
						} else {					//内只剩下半帧数据了
									memcpy((void *) (display_buf),videoFramePicStart + left_lenth,DATA_SIZE_temp - left_lenth);
									now_get_frame_size = DATA_SIZE_temp - left_lenth;//
									now_get_csi_size = 0;	//DATA_SIZE被取完了
									break;
								}
					}
				}else {					//不是第一帧的取数方式
					if((now_get_frame_size>0)&&(now_get_frame_size<real_lenth_temp)){					//取了一半，取剩下一半
						if((DATA_SIZE_temp-now_get_csi_size)>=(real_lenth_temp-now_get_frame_size)){			//可以取到剩下一半
							memcpy((void *) (display_buf+now_get_frame_size),videoFramePicStart+ now_get_csi_size, real_lenth_temp-now_get_frame_size);
//							cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<"i_cs-------"<<i_cs<<"  ,CSI_Frame_Counts"<<CSI_Frame_Counts<<endl;
							if((i_cs%Display_per_frame)==0){
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
								hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,display_buf,bpp_temp);
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
							}
							i_cs=(i_cs++)%40320;
							now_get_csi_size = now_get_csi_size+real_lenth_temp-now_get_frame_size;
							now_get_frame_size= 0;
							if(now_get_csi_size>=DATA_SIZE_temp){
								now_get_csi_size=0;
								break;
							}
						}else{				//取不到剩下的一半
							memcpy((void *) (display_buf+now_get_frame_size),videoFramePicStart+ now_get_csi_size, DATA_SIZE_temp-now_get_csi_size);
							now_get_frame_size=now_get_frame_size+DATA_SIZE_temp-now_get_csi_size;
							now_get_csi_size=0;
							break;
						}
					}else{					//取新的一帧,此时now_get_frame_size=0
					//	************************************************
						if(real_lenth_temp<=(DATA_SIZE_temp-now_get_csi_size)){		//剩下一帧可取完
//								printf("(DATA_SIZE-now_get_csi_size) =%d ,now_get_csi_size=%d\n",DATA_SIZE-now_get_csi_size,now_get_csi_size);
//							memcpy((void *) (display_buf_temp),videoFramePicStart+ now_get_csi_size, real_lenth_temp);
//							cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<"i_cs-------"<<i_cs<<"  ,CSI_Frame_Counts"<<CSI_Frame_Counts<<endl;
							if((i_cs%Display_per_frame)==0){
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
								hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,videoFramePicStart+ now_get_csi_size,bpp_temp);
//								cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
							}
							i_cs=(i_cs++)%40320;
							now_get_csi_size =now_get_csi_size+real_lenth_temp;
							now_get_frame_size = 0;		//一帧拿到了就归零
							if(now_get_csi_size==DATA_SIZE_temp){
								now_get_csi_size=0;
								break;
							}
						}else{					//新的一帧开始取，但是新的一帧只能取一半
							memcpy((void *) (display_buf),videoFramePicStart+ now_get_csi_size, DATA_SIZE_temp-now_get_csi_size);
							now_get_frame_size =DATA_SIZE_temp-now_get_csi_size;
							now_get_csi_size = 0;
							break;
						}



					}


			}

		}			//end while 1

		return 0;
	}



