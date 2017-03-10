
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
#include <iostream>
#include "main.h"
#include "csi_module.h"
#include "loss_test.h"
#include "store_struct.h"
#include "store_thread.h"
#include "io_use.h"
//#include "global.h"

using namespace std;
Csi *csi=NULL;

#define VIDEO_FORMAT	V4L2_PIX_FMT_RGB24
#define BUFFER_COUNT	10

struct v4l2_mxc_offset {
	unsigned int u_offset;
	unsigned int v_offset;
};

//create a struct array to hold the information of requested framebuffers
struct fimc_buffer{
	int length;
	void *start;
}framebuff[BUFFER_COUNT];

struct v4l2_capability cap;
struct v4l2_dbg_chip_ident chip;
struct v4l2_frmsizeenum fsize;
struct v4l2_streamparm parm;
struct v4l2_crop crop;
struct v4l2_format fmt;
struct v4l2_mxc_offset off;
struct v4l2_fmtdesc fmtdesc;
int g_capture_mode = 0;
int g_input = 0;
int g_cap_fmt = V4L2_PIX_FMT_RGB24;
int g_extra_pixel = 0;
char fmtstr[8];
struct v4l2_requestbuffers reqbuf;
struct v4l2_buffer buf;
enum v4l2_buf_type type;
static int i;				//如果不加static，编译的时候会出现重复的定义的错误，为什么还在发现当中。。。。
int ret;
int csi_fd;

Csi::Csi()
{}

Csi::~Csi()
{}

int Csi::init_csi(int g_in_width, int g_in_height, int g_out_width, int g_out_height, int g_top, int g_left, int g_camera_framerate)
{
/*	g_in_width = 344;			//将行限定在344*3=1032内
	g_out_width = 344;*/

/*	g_in_width = 336;			//将行限定在344*3=1032内
	g_out_width = 336;*/

/*	g_in_width = 124;			//将行限定在124*3=372内
	g_out_width = 124;*/

	//open device
	csi_fd = open(CAMERA_DEVICE, O_RDWR);
	if(csi_fd < 0){
		printf("open %s failed.\n", CAMERA_DEVICE);
		return -1;
	}

	//query capability
	ret = ioctl(csi_fd, VIDIOC_QUERYCAP, &cap);
	if(ret < 0){
		printf("VIDIOC_QUERYCAP failed(%d)\n", ret);
		return ret;
	}else{
		printf("Capability Informations:\ndriver: %s\ncard: %d\nbus_info: %d\nversion: %08X\ncapabilities: %08X\n", cap.driver, cap.card, cap.bus_info, cap.version, cap.capabilities);
	}

	if (ioctl(csi_fd, VIDIOC_DBG_G_CHIP_IDENT, &chip))
	{
          printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
          return -1;
	}else{
		printf("sensor chip is %s\n", chip.match.name);
	}

	printf("sensor supported frame size:\n");
	fsize.index = 0;
	while (ioctl(csi_fd, VIDIOC_ENUM_FRAMESIZES, &fsize) >= 0) {
		printf("mode%d: format:%d\tsize:%dx%d\n", fsize.pixel_format,fsize.index,fsize.discrete.width,fsize.discrete.height);
		fsize.index++;
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;

	if (ioctl(csi_fd, VIDIOC_S_PARM, &parm) < 0)
	{
	        printf("VIDIOC_S_PARM failed\n");
	        return -1;
	}

	if (ioctl(csi_fd, VIDIOC_S_INPUT, &g_input) < 0)
	{
		printf("VIDIOC_S_INPUT failed\n");
		return -1;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(csi_fd, VIDIOC_G_CROP, &crop) < 0)
	{
		printf("VIDIOC_G_CROP failed\n");
		return -1;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c.width = g_in_width;
	crop.c.height = g_in_height;
	//crop.c.width = fsize.discrete.width;
	//crop.c.height = fsize.discrete.height;
	crop.c.top = g_top;
	crop.c.left = g_left;
	if (ioctl(csi_fd, VIDIOC_S_CROP, &crop) < 0)
	{
		printf("VIDIOC_S_CROP failed\n");
		return -1;
	}
	printf("crop.c.width=%d\tcrop.c.height=%d\n",crop.c.width,crop.c.height);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = g_cap_fmt;
    fmt.fmt.pix.width = g_out_width;
    fmt.fmt.pix.height = g_out_height;
	//fmt.fmt.pix.width = fsize.discrete.width;
	//fmt.fmt.pix.height = fsize.discrete.height;
    if (g_extra_pixel){
		off.u_offset = (2 * g_extra_pixel + g_out_width) * (g_out_height + g_extra_pixel)
			 - g_extra_pixel + (g_extra_pixel / 2) * ((g_out_width / 2)
			 + g_extra_pixel) + g_extra_pixel / 2;
		off.v_offset = off.u_offset + (g_extra_pixel + g_out_width / 2) *
			((g_out_height / 2) + g_extra_pixel);
    	fmt.fmt.pix.bytesperline = g_out_width + g_extra_pixel * 2;
		fmt.fmt.pix.priv = (unsigned int) &off;
    	fmt.fmt.pix.sizeimage = (g_out_width + g_extra_pixel * 2 )
    		* (g_out_height + g_extra_pixel * 2) * 3 / 2;
	} else {
        fmt.fmt.pix.bytesperline = g_out_width*3;
		fmt.fmt.pix.priv = 0;
    	fmt.fmt.pix.sizeimage = g_out_width*g_out_height*3;
	}
    printf("set format as: with=%d\t height=%d\t bytesperline=%d\t sizeimage=%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height,fmt.fmt.pix.bytesperline,fmt.fmt.pix.sizeimage);

    if (ioctl(csi_fd, VIDIOC_S_FMT, &fmt) < 0)
    {
            printf("set format failed\n");
            return -1;
    }

	//get and print stream format
	ret = ioctl(csi_fd, VIDIOC_G_FMT, &fmt);
	if(ret < 0){
		printf("VIDIOC_G_FMT failed(%d)\n", ret);
		return ret;
	}
	printf("Stream Format Informations:\ntype: %d\nwidth: %d\nheight: %d\n", fmt.type, fmt.fmt.pix.width, fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	printf("pixelformat: %s\nfield: %d\nbytesperline: %d\nsizeimage: %d\ncolorspace: %d\npriv: %d\nraw_date: %s\n", fmtstr, fmt.fmt.pix.field, fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage, fmt.fmt.pix.colorspace,fmt.fmt.pix.priv, fmt.fmt.raw_data);

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;

	if (ioctl(csi_fd, VIDIOC_S_PARM, &parm) < 0)
	{
	        printf("VIDIOC_S_PARM failed\n");
	        return -1;
	}

	return 0;
}

int Csi::start_capture(int buffer_count)
{
	//ask the driver to request frame buffers
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = buffer_count;
	ret = ioctl(csi_fd, VIDIOC_REQBUFS, &reqbuf);
	if(ret < 0){
		printf("VIDIOC_REQBUFS failed(%d)\n", ret);
		return ret;
	}else{
		printf("requst %d framebuffers ok\n", buffer_count);
	}


	for(i = 0; i < buffer_count; i++){
		//see the information of buffer created in the driver
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
//		buf.m.userptr = (unsigned long int)framedata;		//使用这个没有用的变量传递用户空间的指针值到内核空间
//		buf.reserved = (unsigned long int)framedata;
		ret = ioctl(csi_fd, VIDIOC_QUERYBUF, &buf);
		if(ret < 0){
			printf("VIDIOC_QUERYBUF failed(%d)\n", ret);
			return ret;
		}

		//mmap buffer
		framebuff[i].length = buf.length;
//		framebuff[i].start = mmap(0,buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
//		if(framebuff[i].start == MAP_FAILED){
//			printf("mmap(%d)failed\n", i);
//			return -1;
//		}
//		printf("framebuffer %d: address=0x%x, length=%d\n", i, (unsigned int)framebuff[i].start, framebuff[i].length);

		//added buffer into the csi queen
		ret = ioctl(csi_fd, VIDIOC_QBUF, &buf);
		if(ret < 0){
			printf("VIDIOC_QBUF failed(%d)\n", ret);
			return ret;
		}

	}

	//stream on, start to capture the video
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(csi_fd, VIDIOC_STREAMON, &type);
	if(ret < 0){
		printf("VIDIOC_STREAMON failed(%d)\n", ret);
		return ret;
	}
	printf("ready to capturing the frame data.\n");

	return 0;
}


int Csi::abandon_some_frame(unsigned char *to_user){

	buf.reserved = (__u32)to_user;		//使用这个没有用的变量传递用户空间的指针值到内核空间
	buf.input=4;
//	printf("buf.reserved=0x%x",buf.m.userptr);
//	printf("FILE-------%s,func------%s,LINE-------%d\n",__FILE__,__func__,__LINE__);
	ret = ioctl(csi_fd, VIDIOC_DQBUF, &buf);
	if(ret < 0){
		printf("VIDIOC_DQBUF failed(%d)\n", ret);
		return ret;
	}else{
//		printf("FILE-------%s,func------%s,LINE-------%d\n",__FILE__,__func__,__LINE__);
		ret = ioctl(csi_fd, VIDIOC_QBUF, &buf);
		if(ret < 0){
				printf("VIDIOC_DQBUF failed(%d)\n", ret);
					return ret;
		}
//		printf("FILE-------%s,func------%s,LINE-------%d\n",__FILE__,__func__,__LINE__);
	}




return 0;
}

int Csi::capture_one_frame_from_csi(unsigned char *to_user)
{

	buf.reserved = (__u32)to_user;		//使用这个没有用的变量传递用户空间的指针值到内核空间
	buf.input=SIZE_OF_CSI;
//	printf("buf.reserved=0x%x",buf.m.userptr);
//	cout<<"FILE-------"<<__FILE__<<"    ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
	ret = ioctl(csi_fd, VIDIOC_DQBUF, &buf);
	if(ret < 0){
		printf("VIDIOC_DQBUF failed(%d)\n", ret);
		return ret;
	}else{
//		cout<<"FILE-------"<<__FILE__<<"    ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
		ret = ioctl(csi_fd, VIDIOC_QBUF, &buf);
		if(ret < 0){
			printf("VIDIOC_DQBUF failed(%d)\n", ret);
			return ret;
		}
//		cout<<"FILE-------"<<__FILE__<<"    ,func------"<<__func__<<" ,LINE-------"<<__LINE__<<endl;
	}



	return 0;
}



int Csi::stop_capture()
{
	//stream off
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(csi_fd, VIDIOC_STREAMOFF, &type);
	if(ret < 0){
		printf("VIDIOC_STREAMOFF failed(%d)\n", ret);
		return ret;
	}

	//release the resource
//	for(i = 0; i < BUFFER_COUNT; i++){
//		munmap(framebuff[i].start, framebuff[i].length);
//	}

	close(csi_fd);

	return 0;
}


int Csi::get_resolution(){
	printf("Getting resolution...................................\n");
	char *get_resolution_buf=(char *)malloc(SIZE_OF_CSI);
	if(get_resolution_buf==NULL)
		printf("get_resolution_buf malloc error\n");
				unsigned int camera_num_temp;
				unsigned int csi_frame_count_temp;
				unsigned int frame_rates_temp;
				unsigned int xSize_temp;
				unsigned int ySize_temp;
				unsigned int bpp_temp;
				unsigned int max_temp;
				unsigned int min_temp;
				unsigned int position_temp;

	while(1){
		buf.reserved =(__u32)get_resolution_buf;
		buf.input=SIZE_OF_CSI;
		ret = ioctl(csi_fd, VIDIOC_DQBUF, &buf);
		if(ret < 0){
			printf("VIDIOC_DQBUF failed(%d)\n", ret);
			continue;
		}else{
			ret = ioctl(csi_fd, VIDIOC_QBUF, &buf);
			if(ret < 0){
				printf("VIDIOC_DQBUF failed(%d)\n", ret);
				continue;
			}

			camera_num_temp = *(unsigned int *) (get_resolution_buf+ 0);
			csi_frame_count_temp =*(unsigned int *) (get_resolution_buf + 4);
			frame_rates_temp = *(unsigned int *) (get_resolution_buf+ 8);
			xSize_temp = *(unsigned int *) (get_resolution_buf + 12);
			ySize_temp = *(unsigned int *) (get_resolution_buf+ 16);
			bpp_temp = *(unsigned int *) (get_resolution_buf + 20);
			max_temp = *(unsigned int *) (get_resolution_buf + 24);
			min_temp = *(unsigned int *) (get_resolution_buf + 28);
			position_temp = *(unsigned int *) (get_resolution_buf+ 32);
			if ((camera_num_temp>=1)&&(camera_num_temp<=32)&&(bpp_temp==real_FRAME_BPP)&&(xSize_temp<4096)&&(ySize_temp<4096)&&(xSize_temp>50)&&(ySize_temp>50)&&(frame_rates_temp>0)&&(frame_rates_temp<800)) {
				printf("********************************CORRECT******************************************\n");
				printf("camera_num_temp=%d , csi_frame_count_temp = %d , frame_rates_temp =%d \n",camera_num_temp,csi_frame_count_temp,frame_rates_temp);
				printf("xSize_temp=%d 	   , ySize_temp = %d           , bpp_temp =%d \n",xSize_temp,ySize_temp,bpp_temp);
				printf("max_temp=%d 	   , min_temp = %d             , position_temp =%d \n",max_temp,min_temp,position_temp);
				real_X_SIZE_OF_FRAME=xSize_temp;
				real_Y_SIZE_OF_FRAME=ySize_temp;
				Frame_rate=frame_rates_temp;
				real_CAMERA_NUM=camera_num_temp;
				real_FRAME_BPP=bpp_temp;
				if(real_FRAME_BPP>8){
					real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME*2;
				}else{
					real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME;
				}
				DATA_SIZE=SIZE_OF_CSI-SIZE_OF_FPGA_HEAD;
				Display_per_frame=(Frame_rate/25)+1;			//cpu 一秒只能显示25帧
				cout<<"real_lenth = "<<real_lenth<<endl;
				cout<<"DATA_SIZE = "<<DATA_SIZE<<endl<<"Display_per_frame = "<<Display_per_frame<<endl;
				cout<<"**********************************************************************************"<<endl;;

				break;  //!!!
			}
//#ifdef PRINT_SOMETHING
			printf("get_resolution__________WRONG\nContinueing...........\n");
			printf("********************************WRONG*********************************************\n");
			printf("camera_num_temp=%d , csi_frame_count_temp = %d , frame_rates_temp =%d \n",camera_num_temp,csi_frame_count_temp,frame_rates_temp);
			printf("xSize_temp=%d 	   , ySize_temp = %d           , bpp_temp =%d \n",xSize_temp,ySize_temp,bpp_temp);
			printf("max_temp=%d 	   , min_temp = %d             , position_temp =%d \n",max_temp,min_temp,position_temp);
			printf("**********************************************************************************\n");
//#endif
		}
	}


	free(get_resolution_buf);

	return 0;
}



int Csi::change_bits(){
	printf("Changing bits...................................\n");
	char *get_resolution_buf=(char *)malloc(SIZE_OF_CSI);
	if(get_resolution_buf==NULL)
		printf("get_resolution_buf malloc error\n");
				unsigned int camera_num_temp;
				unsigned int csi_frame_count_temp;
				unsigned int frame_rates_temp;
				unsigned int xSize_temp;
				unsigned int ySize_temp;
				unsigned int bpp_temp;
				unsigned int max_temp;
				unsigned int min_temp;
				unsigned int position_temp;

	while(1){
		buf.reserved =(__u32)get_resolution_buf;
		buf.input=SIZE_OF_CSI;
		ret = ioctl(csi_fd, VIDIOC_DQBUF, &buf);
		if(ret < 0){
			printf("VIDIOC_DQBUF failed(%d)\n", ret);
			continue;
		}else{
			ret = ioctl(csi_fd, VIDIOC_QBUF, &buf);
			if(ret < 0){
				printf("VIDIOC_DQBUF failed(%d)\n", ret);
				continue;
			}

			camera_num_temp = *(unsigned int *) (get_resolution_buf+ 0);
			csi_frame_count_temp =*(unsigned int *) (get_resolution_buf + 4);
			frame_rates_temp = *(unsigned int *) (get_resolution_buf+ 8);
			xSize_temp = *(unsigned int *) (get_resolution_buf + 12);
			ySize_temp = *(unsigned int *) (get_resolution_buf+ 16);
			bpp_temp = *(unsigned int *) (get_resolution_buf + 20);
			max_temp = *(unsigned int *) (get_resolution_buf + 24);
			min_temp = *(unsigned int *) (get_resolution_buf + 28);
			position_temp = *(unsigned int *) (get_resolution_buf+ 32);
			if ((camera_num_temp>=1)&&(camera_num_temp<=32)&&(real_FRAME_BPP==bpp_temp)&&(xSize_temp<4096)&&(ySize_temp<4096)&&(xSize_temp>50)&&(ySize_temp>50)&&(frame_rates_temp>0)&&(frame_rates_temp<800)) {
				printf("********************************CORRECT******************************************\n");
				printf("camera_num_temp=%d , csi_frame_count_temp = %d , frame_rates_temp =%d \n",camera_num_temp,csi_frame_count_temp,frame_rates_temp);
				printf("xSize_temp=%d 	   , ySize_temp = %d           , bpp_temp =%d \n",xSize_temp,ySize_temp,bpp_temp);
				printf("max_temp=%d 	   , min_temp = %d             , position_temp =%d \n",max_temp,min_temp,position_temp);
				real_X_SIZE_OF_FRAME=xSize_temp;
				real_Y_SIZE_OF_FRAME=ySize_temp;
				Frame_rate=frame_rates_temp;
				real_CAMERA_NUM=camera_num_temp;
				real_FRAME_BPP=bpp_temp;
				if(real_FRAME_BPP>8){
					real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME*2;
				}else{
					real_lenth=real_X_SIZE_OF_FRAME*real_Y_SIZE_OF_FRAME;
				}
				DATA_SIZE=SIZE_OF_CSI-SIZE_OF_FPGA_HEAD;
				Display_per_frame=(Frame_rate/25)+1;			//cpu 一秒只能显示28帧
				cout<<"real_lenth = "<<real_lenth<<endl;
				cout<<"DATA_SIZE = "<<DATA_SIZE<<endl<<"Display_per_frame = "<<Display_per_frame<<endl;
				cout<<"**********************************************************************************"<<endl;;

				break;  //!!!
			}
		}
	}


	free(get_resolution_buf);

	return 0;
}

