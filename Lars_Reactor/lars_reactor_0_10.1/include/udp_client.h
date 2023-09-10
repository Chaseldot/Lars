#pragma once

#include "net_connection.h"
#include "message.h"
#include "event_loop.h"

class udp_client: public net_connection{
    public:
        udp_client(event_loop *loop, const char* ip, uint16_t port);
        ~udp_client();

        void add_msg_router(int msgid, msg_callback* cb, void *user_data = NULL);

        virtual int send_message(const char *data, int msglen, int msgid);//data-->msg_head-->obuf

        //处理消息
        void do_read();//ibuf-->msg_head-->router-->msg_callback

    private:

        int _sockfd;

        char _read_buf[MESSAGE_LENGTH_LIMIT];//读缓冲区
        char _write_buf[MESSAGE_LENGTH_LIMIT];//写缓冲区

        //事件触发
        event_loop *_loop;

        //消息路由分发
        msg_router _router;
};