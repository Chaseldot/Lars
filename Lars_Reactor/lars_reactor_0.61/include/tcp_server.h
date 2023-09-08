#pragma once

#include <netinet/in.h>
#include <unordered_map>
#include "event_loop.h"
#include "tcp_conn.h"
#include "message.h"

class tcp_server{
    private:
        int _sockfd;//套接字
        struct sockaddr_in _connaddr;//客户端链接地址
        socklen_t _addrlen;//客户端链接地址长度

        event_loop* _loop;//事件循环
    
    public:
        //server的构造函数
        tcp_server(event_loop* loop, const char *ip, uint16_t port);
        //开始提供创建链接服务
        void do_accept();
        //链接对象释放的析构cd 
        ~tcp_server();

    //------------------客户端链接管理部分----------------------
    public:
        static void increase_conn(int connfd, tcp_conn* conn);
        static void decrease_conn(int connfd);
        static void get_conn_num(int* curr_conn);//获取当前链接数
        static std::unordered_map<int, tcp_conn*> conns;//全部已经在线的连接信息

    private:
        //TODO
        //从配置文件中读取
    #define MAX_CONNS 2//最大连接数
        static int _max_conns;//最大client连接数 
        static int _curr_conns;//当前client连接数
        static pthread_mutex_t _conns_mutex;//保护_conns的互斥锁

    //---------------------消息分发路由---------------------
    public:
        static msg_router router;
        void add_msg_router(int msgid, msg_callback* msg_cb, void* user_data = NULL){
            router.register_msg_router(msgid, msg_cb, user_data);
        }
};