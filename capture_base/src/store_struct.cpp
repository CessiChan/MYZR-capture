/*
 * store_struct.cpp
 *
 *  Created on: 2016年12月11日
 *      Author: user
 */

#ifndef STORE_STRUCT_CPP_
#define STORE_STRUCT_CPP_
#include "store_struct.h"

unsigned int Frame_rate=30;
unsigned int real_X_SIZE_OF_FRAME=752;
unsigned int real_Y_SIZE_OF_FRAME=480;
unsigned int real_CAMERA_NUM=1;
unsigned int real_FRAME_BPP=14;
unsigned int real_lenth=360960;
unsigned int DATA_SIZE=360924;


unsigned int SIZE_OF_CSI=2073600;					//一个csi buf的大小
unsigned int SEND_X=1440;
unsigned int SEND_Y=1440;

unsigned int Display_per_frame=1;


int demo_mode_with_display=1;
int is_real_time_display=0;

#endif /* STORE_STRUCT_CPP_ */
