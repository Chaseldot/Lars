#pragma once

#include <thread>//#include <pthread.h>
#include "task_msg.h"
#include "thread_queue.h"

class thread_pool{
    public:
        //构造，初始化线程池, 开辟thread_cnt个
        thread_pool(int thread_cnt);

        //获取一个thread
        thread_queue<task_msg>* get_thread();

    private:

        //_queues是当前thread_pool全部的消息任务队列头指针
        //thread_queue<task_msg> ** _queues; 
        std::vector<thread_queue<task_msg>*> _queues;

        //当前线程池中的线程个数
        int _thread_cnt;

        //已经启动的全部thread编号
        //pthread_t* _tids;
        std::vector<std::thread> _threads;

        //当前选中的线程队列下标
        int _index;
};