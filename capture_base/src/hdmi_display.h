/*
 * hdmi_display.h
 *
 *  Created on: 2016年12月26日
 *      Author: user
 */

#ifndef HDMI_DISPLAY_H_
#define HDMI_DISPLAY_H_
#include <stdio.h>
#include <linux/fb.h>
#include <time.h>
#include <sys/time.h>
#define FB_DEV_NAME 				"/dev/fb0"
typedef struct fb_dev{
	int fd;
	void *pfb;
	int size;
	int xres;
	int yres;
	int bpp;
	struct fb_var_screeninfo fb_var_info;
}fb_dev_t;


extern fb_dev_t display_fd;

//720p显示，1280x720@60
#define  display_x		1024
#define  display_y		720
class hdmi_display{
public:
	hdmi_display();
	~hdmi_display();
	int fb_open(fb_dev_t *fbd);
	int fb_close(fb_dev_t *fbd);
	void lcd_clear(fb_dev_t* fbd);
	void lcd_write_all_bits_frame(int x_size,int y_size,unsigned char *buf,unsigned int bits);
};


extern hdmi_display *hdmi_display_p;



#endif /* HDMI_DISPLAY_H_ */
