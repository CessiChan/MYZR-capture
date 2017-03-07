#ifndef BACK_DISPLAY_H_
#define BACK_DISPLAY_H_
#include <iostream>
#include <string.h>
#include "hdmi_display.h"


extern int is_doing_display;
	int solve_buf(unsigned char *display_buf,char *file_head);
	void *playVideo(void *ptr);
	int read_file(char *file_head,struct dirent *fileName);
	int startDisplayVideo();
	void stopDisplayVideo();




#endif /* BACK_DISPLAY_H_ */










