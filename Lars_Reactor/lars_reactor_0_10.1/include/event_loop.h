#pragma once
/*event_loop事件处理机制*/
#include<sys/epoll.h>
#include<unordered_map>
#include<unordered_set>
#include"event_base.h"

#define MAXEVENTS 10

//map: fd->io_event
typedef std::unordered_map<int, io_event> io_event_map;
//定义指向io_event_map的迭代器
typedef io_event_map::iterator io_event_map_it;
//全部正在监听的fd集合
typedef std::unordered_set<int> listen_fd_set;

class event_loop{
    public:
        event_loop();
        void event_process();//阻塞等待事件发生并处理事件
        void add_io_event(int fd, io_callback *proc, int mask, void *args = nullptr);//添加一个IO事件
        //void del_io_event(int fd);//删除一个IO事件从loop中
        void del_io_event(int fd, int mask=0);//删除一个IO事件的EPOLLIN/EPOLLOUT
    
    private:
        int _epfd;//epoll fd
        io_event_map _io_evs;//当前event_loop监控的fd和对应事件的关系
        listen_fd_set listen_fds;//当前event_loop正在监听的fd集合
        struct epoll_event _fired_evs[MAXEVENTS];//一次性最大处理的事件
};

