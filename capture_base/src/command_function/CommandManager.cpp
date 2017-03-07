/*
 * CommandManager.cpp
 *
 *      Author: WJ
 */

#include "CommandManager.h"

#include <iostream>

CommandManager* CommandManager::instance = NULL;
//queue<int> commandQueue;

CommandManager::CommandManager() {
	// TODO Auto-generated constructor stub
}

CommandManager::~CommandManager() {
	// TODO Auto-generated destructor stub
}

CommandManager* CommandManager::getInstance() {
	if(CommandManager::instance == NULL){
		CommandManager::instance = new CommandManager();
		CommandManager::instance->init();
	}
	return instance;
}

void CommandManager::addOneCommand(int cmd){
	commandQueue.push(cmd);
}

/***
 *
 */
int CommandManager::getNextCommandBlock(){
	int cmd = -1;
	cmd = commandQueue.poll();
	return cmd;
}

void CommandManager::init(){
	cout << "CommandManager init" << endl;
}



//cmd_manager::cmd_manager(){
//	cout<<"cmd_manager Init!!!"<<endl;
//
//}
//
//
//cmd_manager::~cmd_manager(){
//	cout<<"cmd_manager Exit!!!"<<endl;
//
//}
