/*
 * capture_thread.h
 *
 *  Created on: 2016��12��11��
 *      Author: user
 */

#ifndef CAPTURE_THREAD_H_
#define CAPTURE_THREAD_H_


extern int volatile wait_only_display_stop;


void *captureThread(void *ptr);



#endif /* CAPTURE_THREAD_H_ */
