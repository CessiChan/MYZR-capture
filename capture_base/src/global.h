/*
 * global.h
 *
 *  Created on: 2016年12月11日
 *      Author: user
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#define DMA_MEM_NUM				4

#define MALLOC_NUM 				10
#define capture_display_per_csi	2
#define	SSD_SIZE				1.1

#define CAPTURE_VIDEO_STORE_PATH 	"/mnt"
#define KEY_DEVICE_NAME 			"/dev/mykey"
#define FB_DEV_ANME 				"/dev/fb0"
#define PROPERTY_FILE				"/mnt/property.cfg"
#define STORAGE_POSITION 			"/mnt"
#define SPI_NAME 					"/dev/myspi"
#define POWER_DOWN   				"/dev/powerdown_io"
#define	BITS_IO						"/dev/bits_io"

#define real_time_display



//#define USE_UART
//#define USE_CRT
//#define USE_IO_TEST
//#define DISPLAY_IT


//标准采集器配置
#define USE_NET
#define USE_KEY
#define USE_OLED
#define USE_LED
#define USE_BITS_IO

//#define DEBUG
//#define CIS_DEBUG
#ifdef DEBUG
#define printLog printf
#else
#define printLog do {} while (0);
#endif

#ifdef CIS_DEBUG
#define printCSILog printf
#else
#define printCSILog do {} while (0);
#endif


enum SystemStatus {Idle,Capturing,Displaying,Transfering};

#endif /* GLOBAL_H_ */
