/*
 * command.h
 *
 *  Created on: 2016年12月11日
 *      Author: user
 */

#ifndef COMMAND_H_
#define COMMAND_H_

/***
 * comand range 0--255 in byte max region
 * */

#define START_CAPTURE 					1
#define STOP_CAPTURE 					2

#define HDMI_DISPLAY 					3
#define STOP_DISPLAY 					4

#define TRANSFER		 				5
#define STOP_TRANSFER		 			6

#define BOTTON_ADD_BITS 				7
#define LAUNCH_CSI_TEST 				8
#define STOP_CSI_TEST 					9
#define SET_BPP_8						10
#define SET_BPP_10						11
#define SET_BPP_12						12
#define SET_BPP_14						13
#define SET_BPP_16						14

#define DEMO_MODE 						15
#define STOP_DEMO						16
#define POWER_OFF						18
#define MKFS_SDA						19

#define EXIT_ALL						31


#define REFRESH_HEAD					99			//更新FPGA所送的头






#endif /* COMMAND_H_ */
