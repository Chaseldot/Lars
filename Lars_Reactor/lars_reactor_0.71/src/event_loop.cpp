#include"event_loop.h"
#include<assert.h>
#include<cstdio>
#include<cstdlib>

event_loop::event_loop():_epfd(epoll_create1(0)){
    //flag=0,等价于epoll_create
    if(_epfd == -1){
        fprintf(stderr, "epoll_create error\n");
        exit(1);
    }
}

//阻塞等待事件发生并处理事件
void event_loop::event_process(){
    while(true){
        io_event_map_it ev_it;

        int nfds = epoll_wait(_epfd, _fired_evs, MAXEVENTS, 10);
        //nfds: 本次epoll_wait触发的事件数量
        for(int i=0;i<nfds;i++){
            ev_it = _io_evs.find(_fired_evs[i].data.fd);
            assert(ev_it != _io_evs.end());

            io_event *ev = &(ev_it->second);
            
            if(_fired_evs[i].events & EPOLLIN){
                //触发了EPOLLIN事件
                ev->read_callback(this, ev_it->first, ev->rcb_args);
            }else if(_fired_evs[i].events & EPOLLOUT){
                //触发了EPOLLOUT事件
                ev->write_callback(this, ev_it->first, ev->wcb_args);
            }else if(_fired_evs[i].events & (EPOLLHUP | EPOLLERR)){
                //水平触发未处理，可能会出现HUP事件，正常处理读写，没有则清空
                if(ev->read_callback != nullptr){
                    ev->read_callback(this, ev_it->first, ev->rcb_args);
                }else if(ev->write_callback != nullptr){
                    ev->write_callback(this, ev_it->first, ev->wcb_args);
                }else{
                    //没有读写事件，清空
                    fprintf(stderr, "fd %d get error, deleteit from epoll\n", ev_it->first);
                    del_io_event(ev_it->first);
                }
            }
        }
    }
}


/*
 * 这里我们处理的事件机制是
 * 如果EPOLLIN 在mask中， EPOLLOUT就不允许在mask中
 * 如果EPOLLOUT 在mask中， EPOLLIN就不允许在mask中
 * 如果想注册EPOLLIN|EPOLLOUT的事件， 那么就调用add_io_event() 方法两次来注册。
 * */
//添加事件
void event_loop::add_io_event(int fd, io_callback *proc, int mask, void *args){
    int final_mask;
    int op;

    //1. 判断fd是否已经在map中
    io_event_map_it it = _io_evs.find(fd);
    if(it == _io_evs.end()){
        //2. 如果不在map中，那么就添加到map中
        final_mask = mask;
        op = EPOLL_CTL_ADD;
    }else{
        //3. 如果在map中，那么就修改map中的mask
        final_mask = it->second.mask | mask;
        op = EPOLL_CTL_MOD;
    }

    //4. 注册回调函数
    if(mask & EPOLLIN){
        _io_evs[fd].read_callback = proc;
        _io_evs[fd].rcb_args = args;
    }else if(mask & EPOLLOUT){
        _io_evs[fd].write_callback = proc;
        _io_evs[fd].wcb_args = args;
    }

    //5. epoll_ctl添加到epoll堆中
    _io_evs[fd].mask = final_mask;
    //创建原生epoll事件
    struct epoll_event ev;
    ev.events = final_mask;
    ev.data.fd = fd;
    if(epoll_ctl(_epfd, op, fd, &ev) == -1){
        fprintf(stderr, "epoll ctl %d error\n", fd);
        return;
    }
    //6. 将fd添加到监听集合中
    listen_fds.insert(fd);
}
/*
//删除一个io事件从loop中
void event_loop::del_io_event(int fd){
    _io_evs.erase(fd);

    listen_fds.erase(fd);

    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
}
*/
//删除一个io事件的EPOLLIN/EPOLLOUT事件
void event_loop::del_io_event(int fd, int mask){
    //如果没有这个事件则直接返回
    io_event_map_it it = _io_evs.find(fd);
    if(it == _io_evs.end()){
        return;
    }

    int &new_mask = it->second.mask;
    new_mask = new_mask & (~mask);

    if(new_mask == 0){
        //如果没有事件了，那么就删除这个fd
        _io_evs.erase(fd);
        listen_fds.erase(fd);
        epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
    }else{
        //如果还有事件，那么就修改这个fd的事件
        struct epoll_event ev;
        ev.events = new_mask;
        ev.data.fd = fd;
        epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}