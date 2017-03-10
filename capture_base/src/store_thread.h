/*
 * store_thread.h
 *
 *  Created on: 2016Äê12ÔÂ11ÈÕ
 *      Author: user
 */

#ifndef STORE_THREAD_H_
#define STORE_THREAD_H_

#include <iostream>
#include <string>
#include <dirent.h>

using namespace std;

extern unsigned char *display_buf;

void *storeFileThread(void *para_of_pic);
void use_num_to_file(char* currentStoreRootDirName, char* fileName,int num_temp);
int global_cache_display(unsigned char *file_head);

#endif /* STORE_THREAD_H_ */
