/*
 * io_use.cpp
 *
 *  Created on: 2016年12月12日
 *      Author: user
 */

#include "io_use.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <fstream>

static int led_fd=-1;

int led_open(){
	led_fd = open(KEY_LED, O_RDWR);
	if(led_fd < 0){
		printf("open %s fail.\n", KEY_LED);

	}
	return led_fd;
}
int led_lighten(char led_no)			//控制三个led显示灯
{
	write(led_fd, &led_no, 1);
	return 0;
}

