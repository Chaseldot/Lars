#include "thread_pool.h"
#include "event_loop.h"
#include "tcp_conn.h"
#include <unistd.h>
#include <stdio.h>

/*
 * 一旦有task消息过来，这个业务是处理task消息业务的主流程
 *
 * 只要有人调用 thread_queue:: send()方法就会触发次函数
 */
void deal_task_message(event_loop *loop, int fd, void *args){
    //得到是哪个消息队列触发的 
    thread_queue<task_msg>* queue = (thread_queue<task_msg>*)args;

    //将queue中的全部任务取出来
    std::queue<task_msg> tasks;
    queue->recv(tasks);

    while (tasks.empty() != true) {
        task_msg task = tasks.front();

        //弹出一个元素
        tasks.pop();

        if (task.type == task_msg::NEW_CONN) {
            //是一个新建链接的任务
            //并且将这个tcp_conn加入当当前线程的loop中去监听
            tcp_conn *conn = new tcp_conn(task.connfd, loop);
            if (conn == NULL) {
                fprintf(stderr, "in thread new tcp_conn error\n");
                exit(1);
            }

            printf("[thread]: get new connection succ!\n");
        }
        else if (task.type == task_msg::NEW_TASK) {
            //是一个新的普通任务
            //TODO
        } 
        else {
            //其他未识别任务
            fprintf(stderr, "unknow task!\n");
        }
    }
}

//一个线程的主业务main函数
void* thread_main(void* args){
    thread_queue<task_msg> *queue = (thread_queue<task_msg>*)args;

    //每个线程都应该有一个event_loop来监控客户端链接的读写事件
    event_loop *loop = new event_loop();
    if (loop == NULL) {
        fprintf(stderr, "new event_loop error\n");
        exit(1);
    }

    //注册一个触发消息任务读写的callback函数 
    queue->set_loop(loop);
    queue->set_callback(deal_task_message, queue);

    //启动阻塞监听
    loop->event_process();

    return NULL;
}


thread_pool::thread_pool(int thread_cnt): _thread_cnt(thread_cnt), _index(0){

    if (_thread_cnt <= 0) {
        fprintf(stderr, "_thread_cnt < 0\n");
        exit(1);
    }

    //任务队列的个数和线程个数一致
    //_queues = new thread_queue<task_msg>*[thread_cnt];
    //_tids = new pthread_t[thread_cnt];
    _queues.resize(thread_cnt);
    _threads.reserve(thread_cnt);

    // int ret;
    // for (int i = 0; i < thread_cnt; ++i) {
    //     //创建一个线程
    //     printf("create %d thread\n", i);
    //     //给当前线程创建一个任务消息队列
    //     _queues[i] = new thread_queue<task_msg>();
    //     ret = pthread_create(&_threads[i], NULL, thread_main, _queues[i]);
    //     if (ret == -1) {
    //         perror("thread_pool, create thread");
    //         exit(1);
    //     }

    //     //将线程脱离
    //     pthread_detach(_tids[i]);
    // }
    for(int i = 0; i < thread_cnt; i++) {
        //创建一个线程
        printf("create %d thread\n", i);
        //给当前线程创建一个任务消息队列
        _queues[i] = new thread_queue<task_msg>();
        _threads.emplace_back(&thread_main, _queues[i]);//emplace直接在vector中构造对象，thread_main本身就是一个函数对象
        _threads[i].detach();//detach()函数将线程和主线程分离，主线程不用等待子线程结束
    }
}

thread_queue<task_msg>* thread_pool::get_thread(){
    return _queues[_index++ % _thread_cnt];
}