/*
 * store_thread.h
 *
 *  Created on: 2016��12��11��
 *      Author: user
 */

#ifndef STORE_THREAD_H_
#define STORE_THREAD_H_

#include <iostream>
#include <string>
#include <dirent.h>

using namespace std;
void *storeFileThread(void *para_of_pic);
void use_num_to_file(char* currentStoreRootDirName, char* fileName,int num_temp);


#endif /* STORE_THREAD_H_ */
