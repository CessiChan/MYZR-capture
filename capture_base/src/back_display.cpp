#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <malloc.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h> 
#include "back_display.h"
#include "main.h"
#include "global.h"
#include "store_struct.h"
#include "loss_test.h"
using namespace std;

pthread_t tid_play;

int is_doing_display = 0;



volatile bool cancleDisplay = false;

static int fileNumber;
char stroe_path[100];				//瀛樺偍鐨勬枃浠跺す
static struct dirent** namelist;
extern volatile SystemStatus currentSystemStatus;
	int filter_fn(const struct dirent * ent)
	 {
	   if(ent->d_type != DT_REG)
		 return 0;

	   return 1;
	 }

	int getDirFileList(){
	int len;
	len = sprintf(stroe_path, "%s/%s", CAPTURE_VIDEO_STORE_PATH, currentStoreRootDirName);
//	printf("After : %d    \n",savefile_num);
	stroe_path[len] = '\0';
	
		fileNumber = scandir(stroe_path, &namelist, filter_fn, alphasort); //alphasort versionsort
	    if (fileNumber < 0)
	    	return -1;
	    else {
		    return fileNumber;
		 }
	}
	
	
int read_file(char *file_head,struct dirent *fileName){
	int len;
	int ret;
	int fd;
	int size=SIZE_OF_CSI;
	char file_name[100];
	//1 get file name
	len = sprintf(file_name, "%s/%s",stroe_path, fileName->d_name);
	file_name[len] = '\0';
//	cout<<"display path"<<file_name<<endl;
	fd = open(file_name, O_RDONLY);
	if(fd == -1){
		printf("open file error!\r\n");
		return -1;
	}

//	printf("getFrameInfoByFileName fileName = %s \r\n", p_video_file_info->fileName);





	// 2. read file body
	len = 0;
	char *fileHeader = file_head;
	while(len < size){
		ret = read(fd, fileHeader, size - len);
		if(ret > 0){
			len += ret;
			fileHeader += ret;
		}else{
			break;
		}
	}

//	printLog("getFrameInfoByFileName: read file body %d complete\r\n", size);

	close(fd);

	return 0;

		
		
	}
	
	
	void *playVideo(void *ptr){
		int count;
		char *store_buf=(char *)malloc(SIZE_OF_CSI+100);
		if(store_buf==NULL)
			cout<<"store_buf malloc error"<<endl;
		unsigned char *display_buf_1=(unsigned char *)malloc(SIZE_OF_CSI);
		if(display_buf_1==NULL)
			cout<<"display_buf malloc error"<<endl;
		if(currentSystemStatus == Idle){
				cancleDisplay = false;
				currentSystemStatus = Displaying; //鍏ㄥ眬鎺у埗鎾斁
		}
		
		getDirFileList();
		
//		cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
		for(count = 0; count < fileNumber && cancleDisplay != true; count++){ // get every one video file
//		cout<<"FILE NUM="<<fileNumber<<endl;
			read_file(store_buf,namelist[count]);
//			cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
			solve_buf(display_buf_1,store_buf);
	//		cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
		}
//				cout<<"FILE-------"<<__FILE__<<"  ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
		free(store_buf);
		free(display_buf_1);
		is_doing_display = 0;				//娌℃湁杩涜鍥炴斁浜�
		return NULL;
		
	}
	
	
	int solve_buf(unsigned char *display_buf_temp,char *file_head){
//		int i_cs;
		unsigned int left_lenth=0;
		int sleeptime;
		unsigned int csi_frame_count_temp=*(unsigned int *)(file_head+4);
		unsigned int frame_rates_temp=*(unsigned int *)(file_head+8);
		int xSize_temp=*(int *)(file_head+12);
		int ySize_temp=*(int *)(file_head+16);
		unsigned int bpp_temp=*(unsigned int *)(file_head+20);;
//		unsigned int max_temp=p_vide_frame_info->max;
//		unsigned int min_temp=p_vide_frame_info->min;
		unsigned int position_temp=*(unsigned int *)(file_head+32);
		unsigned int DATA_SIZE_temp=SIZE_OF_CSI-SIZE_OF_FPGA_HEAD;
		char *videoFramePicStart=file_head+36;
		unsigned int real_lenth_temp;
		if(bpp_temp==8){
			real_lenth_temp=xSize_temp*ySize_temp;
		}else{
			real_lenth_temp=xSize_temp*ySize_temp*2;
		}

//		printf("csi_frame_count_temp =%d ,frame_rates_temp =%d ,position_temp =%d\n",csi_frame_count_temp,frame_rates_temp, position_temp);
		if(last_csi_count>0){									//判断是否丢帧,last_csi_count初始化为0的，因此第一次不会误进入此处
			if(csi_frame_count_temp==(last_csi_count+1)&&(now_get_frame_size==position_temp)){			//未丢帧
				last_csi_count=csi_frame_count_temp;			//记录信息
			}else{												//丢帧
					init_recount_states();							//放弃一切，重新开始。。。
				}

		}
		/*************************************************处理数据*****************************************
		*int now_block_count=-1;						//用于多头，已经取得第几个block，从0开始
		*unsigned int now_get_frame_size=0;			//取得的数据量
		*unsigned int now_get_csi_size=0;				//csi中已经解析的数据，最大值为DATA_SIZE
		*unsigned int last_csi_count=0;				//上一次取得的CSI的计数
		**************************************************************************************************/

			while (1) {

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
//								printf("FILE :%s ,func : %s ,LINE :%d\n", __FILE__,__func__, __LINE__);
//							memcpy((void *) (display_buf_temp),videoFramePicStart + left_lenth, real_lenth_temp);
//							printf("FILE :%s ,func : %s ,LINE :%d \n", __FILE__,__func__, __LINE__);
							hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,(unsigned char *)(videoFramePicStart + left_lenth),bpp_temp);//閺勫墽銇氱�癸拷
							now_get_csi_size = real_lenth_temp + left_lenth;
							now_get_frame_size = 0;		//记录信息
							sleeptime=400*1000/frame_rates_temp;				//休眠时间
							usleep(sleeptime);
							if (now_get_csi_size == DATA_SIZE_temp) {				//数据刚好取完了
								now_get_csi_size = 0;
								break;
							}
						} else {					//内只剩下半帧数据了
//									printf("FILE :%s ,func : %s ,LINE :%d\n", __FILE__,__func__, __LINE__);
								memcpy((void *) (display_buf_temp),videoFramePicStart + left_lenth,DATA_SIZE_temp - left_lenth);
//									printf("FILE :%s ,func : %s ,LINE :%d\n", __FILE__,__func__, __LINE__);
								now_get_frame_size = DATA_SIZE_temp - left_lenth;//
								now_get_csi_size = 0;	//DATA_SIZE被取完了
								break;
								}
					}
				}else {					//不是第一帧的取数方式
					if((now_get_frame_size>0)&&(now_get_frame_size<real_lenth_temp)){					//取了一半，取剩下一半
						if((DATA_SIZE_temp-now_get_csi_size)>=(real_lenth_temp-now_get_frame_size)){			//可以取到剩下一半
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							memcpy((void *) (display_buf_temp+now_get_frame_size),videoFramePicStart+ now_get_csi_size, real_lenth_temp-now_get_frame_size);
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,display_buf_temp,bpp_temp);
							now_get_csi_size = now_get_csi_size+real_lenth_temp-now_get_frame_size;
							now_get_frame_size= 0;
							sleeptime=400*1000/frame_rates_temp;
							usleep(sleeptime);
							if(now_get_csi_size>=DATA_SIZE_temp){
								now_get_csi_size=0;
								break;
							}
						}else{				//取不到剩下的一半
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							memcpy((void *) (display_buf_temp+now_get_frame_size),videoFramePicStart+ now_get_csi_size, DATA_SIZE_temp-now_get_csi_size);
//							printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							now_get_frame_size=now_get_frame_size+DATA_SIZE_temp-now_get_csi_size;
							now_get_csi_size=0;
							break;
						}
					}else{					//取新的一帧,此时now_get_frame_size=0
					//	************************************************
						if(real_lenth_temp<=(DATA_SIZE_temp-now_get_csi_size)){		//剩下一帧可取完
//								printf("(DATA_SIZE-now_get_csi_size) =%d ,now_get_csi_size=%d\n",DATA_SIZE-now_get_csi_size,now_get_csi_size);
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
//							memcpy((void *) (display_buf_temp),videoFramePicStart+ now_get_csi_size, real_lenth_temp);
	//							printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							hdmi_display_p->lcd_write_all_bits_frame(xSize_temp,ySize_temp,(unsigned char *)(videoFramePicStart+ now_get_csi_size),bpp_temp);//閺勫墽銇氱�癸拷  //閺勫墽銇氱�癸拷
							now_get_csi_size =now_get_csi_size+real_lenth_temp;
							now_get_frame_size = 0;		//一帧拿到了就归零
							sleeptime=400*1000/frame_rates_temp;				//1s閼宠姤妯夌粈锟�60M娑撳秴宕遍妴鍌橈拷鍌橈拷锟�
							usleep(sleeptime);
							if(now_get_csi_size==DATA_SIZE_temp){				//濮濄倖妞傞崚姘偨婵夘偅寮ф稉锟界敮锟�,瀵板牆鐨幆鍛枌閼宠棄顧勬潻娑滎攽閸掓媽绻栨稉锟藉銉礉闂勩倝娼崚姘偨閺勵垱鏆ｉ弫鏉匡拷宥囨畱閺冭泛锟斤拷
								now_get_csi_size=0;
								break;
							}
						}else{					//新的一帧开始取，但是新的一帧只能取一半
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							memcpy((void *) (display_buf_temp),videoFramePicStart+ now_get_csi_size, DATA_SIZE_temp-now_get_csi_size);
//								printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
							now_get_frame_size =DATA_SIZE_temp-now_get_csi_size;
							now_get_csi_size = 0;
							break;
						}



					}


			}

		}			//end while 1

		return 0;
	}
	
	
	int startDisplayVideo(){
		int ret;
		is_doing_display = 1;				//寮�濮嬪洖鏀�
		init_recount_states();
		ret = pthread_create(&tid_play, NULL, playVideo, (void *)NULL);
		if(ret != 0){
			printf("create alive thread error!\r\n");
		}
		return ret;
	}

	void stopDisplayVideo(){
		cancleDisplay = true;
		currentSystemStatus = Idle;
	}
