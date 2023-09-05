#include "reactor_buf.h"
#include<sys/ioctl.h>
#include<cstring>
#include<errno.h>

reactor_buf::reactor_buf():_buf(nullptr){}

reactor_buf::~reactor_buf(){
    clear();
}

const int reactor_buf::length() const{
    return _buf == NULL ? 0 : _buf->length;
}

void reactor_buf::pop(int len){
    _buf->pop(len);

    if(_buf->length == 0){
        clear();
    }
}

void reactor_buf::clear(){
    if(_buf != NULL){
        buf_pool::instance()->revert(_buf);
        _buf = NULL;
    }
}

/*
------->input_buf
*/

//从一个fd中读数据到reactor_buf中
int input_buf::read_data(int fd){
    int need_read;//硬件中有多少需要读

    //一次性读出所有的数据
    //需要给fd设置FIONREAD,
    //得到read缓冲中有多少数据是可以读取的
    if(ioctl(fd, FIONREAD , &need_read) == -1){
        fprintf(stderr, "ioctl FIONREAD\n");
        return -1;
    }
    if(_buf == NULL){//没有就分配
        _buf = buf_pool::instance()->alloc_buf(need_read);
        if(_buf == NULL){
            fprintf(stderr, "no idle buf for alloc\n");
            return -1;
        }
    }else{//判断是否够存
        assert(_buf->head == 0);

        if(_buf->capacity - _buf->length < need_read){
            io_buf* new_buf = buf_pool::instance()->alloc_buf(need_read + _buf->length);
            if(new_buf == NULL){
                fprintf(stderr, "no idle buf for alloc\n");
                return -1;
            }
            new_buf->copy(_buf);
            buf_pool::instance()->revert(_buf);
            _buf = new_buf;
        }
    }

    //读取数据
    int already_read = 0;
    do{
        if(need_read){
            already_read = read(fd, _buf->data + _buf->length, need_read);
        }else{
            already_read = read(fd, _buf->data + _buf->length, m4K);
        }
    }while(already_read == -1 && errno == EINTR);

    if(already_read > 0){
        if(need_read){
            assert(already_read == need_read);
        }
        _buf->length += already_read;
    }

    return already_read;
}

//取出读到的数据
const char* input_buf::data() const{
    return _buf != NULL ? _buf->data + _buf->head : NULL;
}

//重置缓冲区
void input_buf::adjust(){
    if(_buf != NULL){
        _buf->adjust();
    }
}

/*
------->output_buf
*/

//将一段数据写到reactor_buf中
int output_buf::send_data(const char* data, int datalen){
    if(_buf == NULL){
        _buf = buf_pool::instance()->alloc_buf(datalen);
        if(_buf == NULL){
            fprintf(stderr, "no idle buf for alloc\n");
            return -1;
        }
    }else{
        assert(_buf->head == 0);
        if(_buf->capacity - _buf->length < datalen){
            io_buf* new_buf = buf_pool::instance()->alloc_buf(datalen + _buf->length);
            if(new_buf == NULL){
                fprintf(stderr, "no idle buf for alloc\n");
                return -1;
            }
            new_buf->copy(_buf);
            buf_pool::instance()->revert(_buf);
            _buf = new_buf;
        }
    }

    memcpy(_buf->data + _buf->length, data, datalen);
    _buf->length += datalen;
    return 0;
}

//将数据写到fd中
int output_buf::write2fd(int fd){
    assert(_buf != NULL && _buf->head == 0);

    int already_write = 0;
    do{
        already_write = write(fd, _buf->data, _buf->length);
    }while(already_write == -1 && errno == EINTR);

    if(already_write > 0){
        _buf->pop(already_write);
        _buf->adjust();
    }
    //如果fd非阻塞，可能会得到EAGAIN错误
    if(already_write == -1 && errno == EAGAIN){
        already_write = 0;//不是错误，表示目前资源不足
    }
    return already_write;
}