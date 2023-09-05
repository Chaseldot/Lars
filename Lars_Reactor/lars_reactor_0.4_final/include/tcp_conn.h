#pragma once

#include"reactor_buf.h"
#include"event_loop.h"

//一个tcp的连接信息
class tcp_conn{
    public:
        tcp_conn(int connfd, event_loop* loop);//构造函数
        void do_read();//处理读业务
        void do_write();//处理写业务
        void clean_conn();//清除连接
        int send_message(const char* data, int msglen, int msgid);//发送消息

    private:
        int _connfd;//连接的fd
        event_loop* _loop;//所属的event_loop
        output_buf obuf;//输出缓冲区
        input_buf ibuf;//输入缓冲区
};
