/*
 * key_module.cpp
 *
 *  Created on: 2017Äê2ÔÂ16ÈÕ
 *      Author: user
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <fstream>

#include "key_module.h"
#include "../command.h"
#include "../../global.h"
using namespace std;

extern volatile int currentSystemStatus;

//int My_key::key_fd;
//struct pollfd My_key::key_fds;

My_key::My_key(){
	key_fd = open(KEY_DEVICE_NAME, O_RDWR);
	if(key_fd < 0){
		cout<<"open my_key failed"<<endl;
	}
	cout<<"open %s success.\n"<<endl;
	key_fds.fd = key_fd;
	key_fds.events = POLLIN;



}

My_key::~ My_key(){
	if(key_fd != 0){
		close(key_fd);
	}
}


int My_key::read_key(){

	int key_value;
	int ret;
		ret = poll(&key_fds, 1, 5000);
		if(ret == 0){
			// time out do nothing
			return -1;
		}else{
			read(key_fd, &key_value, sizeof(key_value));
		}
		switch(key_value){
		case 1:
			if(currentSystemStatus==Capturing)
				key_value=STOP_CAPTURE;
			else
				key_value=START_CAPTURE;
			break;
		case 2:
			if(currentSystemStatus==Displaying)
				key_value=STOP_DISPLAY;
			else
				key_value=HDMI_DISPLAY;
			break;
		case 3:
			if(currentSystemStatus==Transfering)
				key_value=STOP_TRANSFER;
			else
				key_value=TRANSFER;
			break;
		case 4:
			key_value=BOTTON_ADD_BITS;
			break;
		default:
			return -1;
			break;


		}

	return key_value;

}




