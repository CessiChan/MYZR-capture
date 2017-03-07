/*
 * CommandReceiverThread.cpp
 *
 *  Created on: 2016楠烇拷9閺堬拷10閺冿拷
 *      Author: WJ
 */
#include "net_thread.h"

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define DEFAULT_PORT 8010

#include "./../CommandManager.h"

using namespace std;

static bool isContinue = true;

static CommandManager* commandManagerInstance;

void *doNetwork(void *arg);
void *command_receiver_thread(void *arg)
{
	int ret;
	int cmd = 0;
	int socket_fd, connect_fd;
	struct sockaddr_in servaddr;
	pthread_t consumeClientConnect;
	cpu_set_t mask;  //CPU核的集合
	cpu_set_t get;	 //获取在集合中的CPU
	int a=2;		//运行在cpu1上
	CPU_ZERO(&mask);	//置空
	CPU_SET(a,&mask);	//设置亲和力值
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//设置线程CPU亲和力
	{
		printf("warning: could not set CPU affinity, continuing...\n");
	}
	commandManagerInstance = CommandManager::getInstance();
	// create one local network service to accept client connect
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(DEFAULT_PORT);

	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		cerr << "Create Socket error: " << strerror(errno) << endl;
	//	return -1;
	}
	if(bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		cerr << "bind local cmd service failed, error: " << strerror(errno) << endl;
		//return -1;
	}

	if(listen(socket_fd, 1) == -1){
		cerr << "listen cmd service on socket failed, error: " << strerror(errno) << endl;
	}

	while(1){
		if((connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){
			cerr << "cmd service accept error: " << strerror(errno) << endl;
			continue;
		}
		ret = pthread_create(&consumeClientConnect, NULL, doNetwork, &connect_fd);
	}

	close(socket_fd);
	return NULL;
}

void *doNetwork(void *arg){
	char buffer[1];
	int n;
	int cmd;
	int connect_fd = *((int *)arg);

	while(1){
		n = recv(connect_fd, buffer, 1, 0);
		if(n == -1){
			cout << "client break out" << endl;
			break;
		}

		if(buffer[0] == 0x55){
			n = recv(connect_fd, buffer, 1, 0);
			if(n == -1){
				cout << "client break out" << endl;
				break;
			}

			if((0xFF & buffer[0]) == 0xaa){
				n = recv(connect_fd, buffer, 2, 0);
				if(n == -1){
					cout << "client break out" << endl;
					break;
				}
				int a = 0x55 + 0xaa + buffer[0];
				char calcualte = a & 0xFF;
				if(buffer[1] == calcualte){
					cmd = (0xFF & buffer[0]);
//					cout << "cmd = " << cmd << endl;
					commandManagerInstance->addOneCommand(cmd);
				}
			}
		}
	}
	return NULL;
}

void closeNetCommandReceiver(){
	isContinue = false;
}

