#pragma once 
#include <unordered_map>
#include <cstdio>

//解决tcp粘包问题的消息头
struct msg_head{
    int msgid; //消息id
    int msglen; //消息长度
};

//消息头的二进制长度，固定数
#define MESSAGE_HEAD_LEN 8

//消息头+消息体的最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

//msg业务回调函数原型
//===================消息分发路由机制====================
class net_connection;
typedef void msg_callback(const char* data, uint32_t len, int msgid, net_connection* client, void* user_data);

//消息路由分发机制
class msg_router{
    public:
        msg_router():_router(), _args(){
            printf("msg router init ...\n");
        }

        //注册消息业务路由
        int register_msg_router(int msgid, msg_callback* msg_cb, void* user_data){
            if(msg_cb == nullptr){
                fprintf(stderr, "msg_cb is nullptr\n");
                return -1;
            }

            if(_router.find(msgid) != _router.end()){
                fprintf(stderr, "msgid %d is exist!\n", msgid);
                return -1;
            }

            _router[msgid] = msg_cb;
            _args[msgid] = user_data;
            return 0;
        }

        //调用注册的业务路由
        void call(int msgid, uint32_t msglen, const char* data, net_connection* net_conn){
            printf("call msgid = %d\n", msgid);
            //判断msgid对应的回调函数是否注册
            if(_router.find(msgid) == _router.end()){
                fprintf(stderr, "msgid %d is not registered!\n", msgid);
                return ;
            }

            msg_callback* msg_cb = _router[msgid];
            void* user_data = _args[msgid];
            msg_cb(data, msglen, msgid, net_conn, user_data);
            printf("=======\n");
        }

    private:
        //路由表 针对消息的路由分发，key为msgID, value为注册的回调业务函数
        std::unordered_map<int, msg_callback*> _router;
        //路由表的参数 回调业务函数对应的参数，key为msgID, value为对应的参数
        std::unordered_map<int, void*> _args;
};



