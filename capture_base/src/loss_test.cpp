/*
 * loss_test.cpp
 *
 *  Created on: 2016年12月10日
 *      Author: user
 */
#include "loss_test.h"
#include <string.h>

unsigned int savefile_num=0;
unsigned int save_loss[CSI_DISCADE_NUM];
unsigned int csi_loss[CSI_DISCADE_NUM];
unsigned int save_loss_count=0;
unsigned int save_last_count=0;

unsigned int csi_loss_frame_count=0;
unsigned int first_frame=0;
unsigned int last_frame=0;
unsigned int csi_get_num=0;

//分解存储块的状态
int now_block_count=-1;						//用于多头，已经取得第几个block，从0开始
unsigned int now_get_frame_size=0;			//取得的数据量
unsigned int now_get_csi_size=0;				//csi中已经解析的数据,去掉头，也就是第一行
unsigned int last_csi_count=0;				//上一次取得的CSI的计数


double	memcpy_time_total=0;
int init_recount_states(){
	now_block_count = -1;				//用于多头，已经取得第几个block，从0开始
	now_get_frame_size = 0;			//取得的数据量
	now_get_csi_size = 0;
	last_csi_count = 0;
	csi_loss_frame_count = 0;
	savefile_num = 0;
	save_last_count = 0;
	save_loss_count = 0;
	csi_get_num = 0;
	 return 0;
}



TimeTool *csi_time_tool=NULL;
TimeTool *save_time_tool=NULL;
TimeTool *memcpy_time_tool=NULL;


TimeTool::TimeTool()
    {
        reset();
    }
	 void TimeTool:: begin()
    {
        gettimeofday(&m_begin, NULL);
    }
	 void TimeTool::end()
    {
        gettimeofday(&m_end, NULL);
    }
	 void TimeTool::reset()
    {
        memset(&m_begin, 0, sizeof(struct timeval));
        memset(&m_end, 0, sizeof(struct timeval));
    }
	double TimeTool::getInterval()
    {
        if (m_end.tv_usec < m_begin.tv_usec)
        {
            m_end.tv_usec += 1000000;
            m_end.tv_sec = m_end.tv_sec - 1;
        }
        return (m_end.tv_sec - m_begin.tv_sec) + (m_end.tv_usec - m_begin.tv_usec) / 1000000.0;
    }
