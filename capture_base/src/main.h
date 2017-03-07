/*
 * main.h
 *
 *  Created on: 2016Äê12ÔÂ11ÈÕ
 *      Author: user
 */

#ifndef MAIN_H_
#define MAIN_H_



extern char currentStoreRootDirName[60];
extern int volatile isCapturing;
int getIdleImageCache();
int getWritedImageCache();
void doStopCaptureAction();
int is_all_idle();
int data_buf_count();


#endif /* MAIN_H_ */
