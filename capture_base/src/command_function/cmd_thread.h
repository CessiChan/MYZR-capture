/*
 * cmd_thread.h
 *
 *  Created on: 2017��1��9��
 *      Author: user
 */

#ifndef CMD_THREAD_H_
#define CMD_THREAD_H_

void * uart_cmd_thread(void *arg);
void * key_cmd_thread(void *arg);
void * crt_cmd_thread(void *arg);

#endif /* CMD_THREAD_H_ */
