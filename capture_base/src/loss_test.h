/*
 * loss_test.h
 *
 *  Created on: 2016年12月10日
 *      Author: user
 */

#ifndef LOSS_TEST_H_
#define LOSS_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <dirent.h>
#include <sys/time.h>

using namespace std;

#define CSI_DISCADE_NUM   30 		//前面丢弃的帧数
extern unsigned int savefile_num;
extern unsigned int save_loss[CSI_DISCADE_NUM];
extern unsigned int csi_loss[CSI_DISCADE_NUM];
extern unsigned int save_loss_count;
extern unsigned int save_last_count;
extern unsigned int csi_loss_frame_count;
extern unsigned int first_frame;
extern unsigned int csi_get_num;



//分解存储块的状态
extern int now_block_count;						//用于多头，已经取得第几个block，从0开始
extern unsigned int now_get_frame_size;			//取得的数据量
extern unsigned int now_get_csi_size;				//csi中已经解析的数据
extern unsigned int last_csi_count;				//上一次取得的CSI的计数
extern unsigned int last_frame;


extern double	memcpy_time_total;
#define MEMCPY_SIZE			27*1024*1024



int init_recount_states();



class TimeTool
{
    public:
    TimeTool();
	void begin();
	void end();
	void reset();
	double getInterval();
	private:
    struct timeval m_begin;
    struct timeval m_end;
};



extern TimeTool *csi_time_tool;
extern TimeTool *save_time_tool;
extern TimeTool *memcpy_time_tool;


#endif /* LOSS_TEST_H_ */
