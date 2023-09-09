#pragma once 
#include"io_buf.h"
#include"buf_pool.h"
#include<assert.h>
#include<unistd.h>
#include<sys/uio.h>

/*
* 给业务层提供最后的最后tcp_buffer结构
*/
class reactor_buf{
    public:
        reactor_buf();
        ~reactor_buf();

        const int length() const;
        void pop(int len);
        void clear();
    protected:
        io_buf* _buf;
};

class input_buf : public reactor_buf{
    public:
        //从一个fd中读取数据到reactor_buf中
        int read_data(int fd);
        //取出读取的数据
        const char* data() const;
        //重置缓冲区
        void adjust();
};

class output_buf : public reactor_buf{
    public:
        //将一段数据 写到一个reactor_buf中
        int send_data(const char* data, int datalen);
        //将reactor_buf中的数据写到一个fd中
        int write2fd(int fd);
};
