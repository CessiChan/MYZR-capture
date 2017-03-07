/*
 * store_struct.h
 *
 *  Created on: 2016年12月11日
 *      Author: user
 */

#ifndef STORE_STRUCT_H_
#define STORE_STRUCT_H_
#include <linux/fb.h>
#include <time.h>
#include <sys/time.h>
#define SIZE_OF_FPGA_HEAD   36
extern unsigned int Frame_rate;
extern unsigned int real_X_SIZE_OF_FRAME;
extern unsigned int real_Y_SIZE_OF_FRAME;
extern unsigned int real_CAMERA_NUM;
extern unsigned int real_FRAME_BPP;
extern unsigned int real_lenth;
extern unsigned int DATA_SIZE;

extern unsigned int Display_per_frame;
extern unsigned int CSI_Frame_Counts;   //一个CSI内存块帧数
extern int demo_mode_with_display;
extern int is_real_time_display;

struct csi_frame_head{
	unsigned int camera_num;			//0
	unsigned int csi_frame_count;		//4
	unsigned int frame_rates;			//8
	unsigned int xSize;					//12
	unsigned int ySize;					//16
	unsigned int bpp;					//20
	unsigned int max;					//24
	unsigned int min;					//28
	unsigned int position;				//32		摄像头一帧中第多少个像素点
};


struct csi_block_head{
	unsigned short camera_num;
	unsigned short block_count;
	unsigned short real_frame_count;
	unsigned short max;
	unsigned short min;
};




/******************************************************************************************************************************
*  video_file_header*  fileData
*   				***********************************************************************************************************
*					*	one frame data					 	*	one frame data					 	*
*					*********************************************************************************
*					*	video_frame_header	#	frame_data 	*	video_frame_header	#	frame_data 	*
/******************************************************************************************************************************/


#endif /* STORE_STRUCT_H_ */
