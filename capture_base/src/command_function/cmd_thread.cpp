/*
 * cmd_thread.cpp
 *
 *  Created on: 2017年1月9日
 *      Author: user
 */

#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "cmd_thread.h"
#include "uart_module/uart_control.h"
#include "key_module/key_module.h"
#include "command.h"
#include "CommandManager.h"
using namespace std;
void * crt_cmd_thread(void *arg){
	int command;
	CommandManager *commandManager = CommandManager::getInstance();
	while(1){
		scanf("%d", &command);
		commandManager->addOneCommand(command);
	}

	return NULL;
}

void * uart_cmd_thread(void *arg){
	int command;
	CommandManager *commandManager = CommandManager::getInstance();
//	uart_cotrol_p=new(uart_message_deal);
	while(1){
		command=uart_cotrol_p->deal_message();			//cmd >0x1F才需要转发
		commandManager->addOneCommand(command);
	}
	return NULL;
}


void * key_cmd_thread(void *arg){
	int command;
	My_key *my_key_p=new (My_key);
	CommandManager *commandManager = CommandManager::getInstance();
	while(1){
		command=my_key_p->read_key();
		if(command>0)
			commandManager->addOneCommand(command);
	}

	return NULL;
}
