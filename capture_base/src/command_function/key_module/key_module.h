/*
 * key_module.h
 *
 *  Created on: 2017Äê2ÔÂ16ÈÕ
 *      Author: user
 */

#ifndef COMMAND_FUNCTION_KEY_MODULE_KEY_MODULE_H_
#define COMMAND_FUNCTION_KEY_MODULE_KEY_MODULE_H_

#include <stdio.h>
#include <poll.h>

class My_key{
public:
	My_key();
	~My_key();
	int read_key();
private:
	int key_fd;
	struct pollfd key_fds;
};





#endif /* COMMAND_FUNCTION_KEY_MODULE_KEY_MODULE_H_ */
