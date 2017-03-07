/*
 * csi_module.h
 *
 *  Created on: 2016年1月22日
 *      Author: wjt
 */

#ifndef CSI_MODULE_H_
#define CSI_MODULE_H_
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "global.h"
#include "capture_thread.h"

#define CAMERA_DEVICE 	"/dev/video0"
extern int csi_video_fd;//存储csi设备信息

class Csi{
public:
	Csi();
	virtual ~Csi();

	int init_csi(int g_in_width, int g_in_height, int g_out_width, int g_out_height, int g_top, int g_left, int g_camera_framerate);
	int start_capture(int buffer_count);
	int abandon_some_frame(unsigned char *to_user);
	int capture_one_frame_from_csi(unsigned char *to_user);
	int stop_capture();
	int get_resolution();
	int change_bits();
};
extern Csi *csi;
#endif /* CSI_MODULE_H_ */


