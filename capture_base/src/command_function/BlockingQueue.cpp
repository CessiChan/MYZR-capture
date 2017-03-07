/*
 * BlockingQueue.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: earljwang
 */

#include "BlockingQueue.h"

BlockingQueue::BlockingQueue()
{
    this->capacity = DEFAULT_QUEUE_SIZE;
    this->number = 0;
    queue = new int[capacity];
    head = 0,tail = 0;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&notFull,NULL);
    pthread_cond_init(&notEmpty,NULL);
}

BlockingQueue::BlockingQueue(int capacity)
{
	if(capacity < 1){
		cerr << "BlockingQueue: capacity must bigger than 0" << endl;
		return;
	}
    this->capacity = capacity;
    this->number = 0;
    queue = new int[capacity];
//    cout << "capacity " << sizeof(queue) << endl;
    head = 0,tail = 0;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&notFull,NULL);
    pthread_cond_init(&notEmpty,NULL);

}

BlockingQueue::~BlockingQueue()
{
    this->capacity = 0;
    head = 0,tail = 0;
    delete queue;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&notFull);
    pthread_cond_destroy(&notEmpty);
}

bool BlockingQueue::push(int item)
{
    pthread_mutex_lock(&mutex);
//    cout << "you want push " << item << endl;

    while(this->number == this->capacity)//is full
    {
//        cout << "is full,wait..." << endl;
        // push wait
        pthread_cond_wait(&notFull,&mutex);
//        cout << "not full,unlock" << endl;
    }
    // do push action
    number += 1;
    queue[head] = item;
	head = (head + 1) % capacity;
//	cout << "push " << item << endl;
	//wake up poll thread
//	cout << "wake up poll thread" << endl;
	pthread_cond_signal(&notEmpty);

	pthread_mutex_unlock(&mutex);

	return true;

}

int BlockingQueue::poll()
{
    pthread_mutex_lock(&mutex);
    int ret = 0;
    while(number == 0) // is empty
    {
//        cout << "is empty,wait..." << endl;
        //poll wait
        pthread_cond_wait(&notEmpty,&mutex);
//        cout << "not empty,unlock..." << endl;
    }

    // do pull action
    number -= 1;
	ret = queue[tail];
	tail = (tail + 1) % capacity;
//	cout << "take " << ret << endl;
	//wake up push thread
//	cout << "wake up push thread" << endl;
	pthread_cond_signal(&notFull);

	pthread_mutex_unlock(&mutex);
	return ret;

}
