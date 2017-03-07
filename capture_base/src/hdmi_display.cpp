/*
 * hdmi_display.cpp
 *
 *  Created on: 2016年12月26日
 *      Author: user
 */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <iostream>
#include "loss_test.h"
#include "hdmi_display.h"

using namespace std;
fb_dev_t display_fd;
hdmi_display *hdmi_display_p=NULL;

hdmi_display::hdmi_display(){
	cout<<"Hdmi init !"<<endl;
	int hdmi_open_ret;
	hdmi_open_ret = fb_open(&display_fd);
	if (hdmi_open_ret == -1) {
		printf("NO HDMI DEVICE \n");
	}

}


hdmi_display::~hdmi_display(){
	cout<<"Hdmi exit !"<<endl;
	fb_close(&display_fd);
}

int hdmi_display::fb_open(fb_dev_t *fbd)
{
	struct fb_fix_screeninfo fb_fix;
//	cout<<"FILE :"<<__FILE__<<"  func :"<<__func__<<"	LINE :"<<__LINE__<<endl;
	fbd->fd = open(FB_DEV_NAME, O_RDWR);
	printf("fbd->fd=%d.\n", fbd->fd);
	if(fbd->fd < 0){
		printf("open %s fail.\n", FB_DEV_NAME);
		return -1;
	}else{
		printf("%s open.\n", FB_DEV_NAME);
	}


	//获取LCD可变参数
	ioctl(fbd->fd,FBIOGET_VSCREENINFO,&fbd->fb_var_info);
	if (-1 == ioctl(fbd->fd, FBIOGET_FSCREENINFO, &fb_fix)) {
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
		return -1;
	}
//	printLog("fb_fix : smem_lenn = %d type = %d line_length = %d \r\n", fb_fix.smem_len, fb_fix.type, fb_fix.line_length);

	if(fbd->fb_var_info.yres_virtual == fbd->fb_var_info.yres){
		fbd->fb_var_info.yres_virtual = fbd->fb_var_info.yres * 2;
	}
	ioctl(fbd->fd,FBIOPUT_VSCREENINFO,&fbd->fb_var_info);
	ioctl(fbd->fd,FBIOGET_VSCREENINFO,&fbd->fb_var_info);
	fbd->size = fbd->fb_var_info.xres_virtual * fbd->fb_var_info.yres_virtual * fbd->fb_var_info.bits_per_pixel / 8;
	fbd->xres = fbd->fb_var_info.xres;
	fbd->yres = fbd->fb_var_info.yres;
	fbd->bpp = fbd->fb_var_info.bits_per_pixel;

	printf("%dx%d,%dbpp,screensie = %d \n",fbd->xres, fbd->yres, fbd->bpp, fbd->size);
	printf("xres_virtual = %d yres_virtual = %d \r\n", fbd->fb_var_info.xres_virtual, fbd->fb_var_info.yres_virtual);

//	printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
	//map the device to memory
	fbd->pfb=mmap(0,fbd->size,PROT_READ|PROT_WRITE,MAP_SHARED,fbd->fd,0);
//	printf("FILE :%s ,func : %s ,LINE :%d\n",__FILE__,__func__,__LINE__);
	if((int)fbd->pfb==-1)
	{
		printf("Error:fail to map framebuffer device to memory.\n");
		//_exit(EXIT_FAILURE);
		return -1;
	}
	return 0;

}


int hdmi_display::fb_close(fb_dev_t *fbd)
{
	munmap(fbd->pfb,fbd->size);
	close(fbd->fd);
	return 0;

}


void hdmi_display::lcd_write_all_bits_frame(int x_size,int y_size,unsigned char *buf,unsigned int bits){

	int y_temp;
	int x_temp;
//	int lineStart;
		// do display
//	if (ioctl(display_fd.fd, FBIO_WAITFORVSYNC, 0) ==-1 ) {
//		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
//	}
//	TimeTool *time_test=new(TimeTool);
//	time_test->begin();
	if(bits==8){
		for(y_temp = 0; y_temp < y_size; y_temp++){
//			lineStart = y_temp*x_size;
			for( x_temp = 0; x_temp < x_size; x_temp++){
				*(int *)((char *)(display_fd.pfb)+(y_temp*display_fd.xres + x_temp)*3) = buf[y_temp*x_size + x_temp] * 0x010101;
			}
		}
	}else{
		char higher;
		char lower;
		for(y_temp = 0; y_temp < y_size; y_temp++){
//			lineStart = y_temp*x_size;
			for( x_temp = 0; x_temp < x_size; x_temp++){
//				higher= (buf[2*(lineStart + x_temp)+1])<<(16-bits);
//				lower= (buf[2*(lineStart + x_temp)])>>(bits-8);
//				cout<<"display_fd.xres = "<<display_fd.xres<<endl;
				*(int *)((char *)(display_fd.pfb)+(y_temp*display_fd.xres + x_temp)*3) = (((buf[2*(y_temp*x_size + x_temp)+1])<<(16-bits)) | ((buf[2*(y_temp*x_size + x_temp)])>>(bits-8))) * 0x010101;
			}
		}


	}
//	time_test->end();
//	double time=time_test->getInterval();
//
//	cout<<"time is "<<time<<"s"<<endl;
//	delete(time_test);

}
