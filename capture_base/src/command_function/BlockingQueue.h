/*
 * BlockingQueue.h
 *
 *  Created on: 2016年9月12日
 *      Author: earljwang
 */

#ifndef CONTAINER_BLOCKINGQUEUE_H_
#define CONTAINER_BLOCKINGQUEUE_H_

#include <iostream>
#include <pthread.h>

#define DEFAULT_QUEUE_SIZE 100

using namespace std;

//template <typename T >
class BlockingQueue
{
public:
    BlockingQueue();
    BlockingQueue(int capacity);
    ~BlockingQueue();

    bool push(int item);
    int poll();
    void clear();
    bool full()const;
    bool empty()const;

private:
    int capacity;
    volatile int number;
    int* queue;
    int head,tail;
    pthread_mutex_t mutex;
    pthread_cond_t notFull,notEmpty;
};

#endif /* CONTAINER_BLOCKINGQUEUE_H_ */
