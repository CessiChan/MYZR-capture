/*
 * CommandManager.h
 *
 *  Created on: 2016骞�9鏈�10鏃�
 *      Author: WJ
 */

#ifndef COMMAND_COMMANDMANAGER_H_
#define COMMAND_COMMANDMANAGER_H_

#include "BlockingQueue.h"

using namespace std;

class CommandManager {
public:
	CommandManager();
	virtual ~CommandManager();
	static CommandManager* getInstance();
	void addOneCommand(int cmd);
	int getNextCommandBlock();

private:
	static CommandManager* instance;
	BlockingQueue commandQueue;
	void init();

};



//class cmd_manager{
//public:
//	cmd_manager();
//	virtual ~cmd_manager();
//	void addOneCommand(int cmd);
//	int getNextCommandBlock();
//private:
//		BlockingQueue commandQueue;
//};


#endif /* COMMAND_COMMANDMANAGER_H_ */
