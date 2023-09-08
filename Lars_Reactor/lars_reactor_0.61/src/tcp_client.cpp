#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "tcp_client.h"

static void write_callback(event_loop* loop, int fd, void* args){
    tcp_client *cli = (tcp_client*)args;
    cli->do_write();
}

static void read_callback(event_loop* loop, int fd, void* args){
    tcp_client *cli = (tcp_client*)args;
    cli->do_read();
}

//判断链接是否是创建链接，主要是针对非阻塞socket 返回EINPROGRESS错误
static void connect_delay(event_loop *loop, int fd, void *args){
    tcp_client *cli = (tcp_client*)args;
    loop->del_io_event(fd);

    int result = 0;
    socklen_t result_len = sizeof(result);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if (result == 0) {
        //链接是建立成功的
        cli->connected = true;

        printf("connect %s:%d succ!\n", inet_ntoa(cli->_server_addr.sin_addr), ntohs(cli->_server_addr.sin_port));

				// ================ 发送msgid：1 =====
        //建立连接成功之后，主动发送send_message
        const char *msg = "hello lars!";
        int msgid = 1;
        cli->send_message(msg, strlen(msg), msgid);

      	// ================ 发送msgid：2 =====
        const char *msg2 = "hello Aceld!";
        msgid = 2;
        cli->send_message(msg2, strlen(msg2), msgid);
				// ================

        loop->add_io_event(fd, read_callback, EPOLLIN, cli);

        if (cli->_obuf.length != 0) {
            //输出缓冲有数据可写
            loop->add_io_event(fd, write_callback, EPOLLOUT, cli);
        }
    }else {
        //链接创建失败
        fprintf(stderr, "connection %s:%d error\n", inet_ntoa(cli->_server_addr.sin_addr), ntohs(cli->_server_addr.sin_port));
    }
}

tcp_client::tcp_client(event_loop* loop, const char* ip, unsigned short port, const char* name):_ibuf(4194304), _obuf(4194304){
    _sockfd = -1;
    _msg_callback = nullptr;
    _name = name;
    _loop = loop;
    
    bzero(&_server_addr, sizeof(_server_addr));
    
    _server_addr.sin_family = AF_INET; 
    inet_aton(ip, &_server_addr.sin_addr);
    _server_addr.sin_port = htons(port);

    _addrlen = sizeof(_server_addr);

    this->do_connect();///////////创建链接
}

//创建链接
void tcp_client::do_connect(){
    if(_sockfd != -1){
        close(_sockfd);
    }

    //创建套接字
    _sockfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, IPPROTO_TCP);
    if(_sockfd == -1){
        fprintf(stderr, "create tcp client socket error\n");
        return ;
    }

    int ret = connect(_sockfd, (const struct sockaddr*)&_server_addr, _addrlen);
    if(ret == 0){
        //链接成功
        connected = true;
        //注册读回调
        _loop->add_io_event(_sockfd, read_callback, EPOLLIN, this);
        //如果写缓冲区有数据，也注册写回调
        if(this->_obuf.length != 0){
            _loop->add_io_event(_sockfd, write_callback, EPOLLOUT, this);
        }

        printf("connect %s:%d succ!\n", inet_ntoa(_server_addr.sin_addr), ntohs(_server_addr.sin_port));
    }else if(ret == -1 && errno == EINPROGRESS){
        //fd是非阻塞的，可能会出现这个错误，但是并不表示链接创建失败
        //如果fd是可写状态，则为链接是创建成功的
        fprintf(stderr, "do_connect EINPROGRESS\n");

        //让event_loop去触发一个创建判断链接业务 用EPOLLOUT事件立刻触发
        _loop->add_io_event(_sockfd, connect_delay, EPOLLOUT, this);
    }else{
        fprintf(stderr, "connect %s:%d error\n", inet_ntoa(_server_addr.sin_addr), ntohs(_server_addr.sin_port));
        exit(1);
    }
}

//处理读业务  socket套接字--->输入缓冲区
int tcp_client:: do_read(){
    //确定已经成功进行连接
    assert(connected == true);
    //1. 一次性全部读出来

        //得到缓冲区内有多少字节要被读取，然后将字节数放入b里面
    int need_read = 0;
    if(ioctl(_sockfd, FIONREAD, &need_read) == -1){
        fprintf(stderr, "ioctl FIONREAD error\n");
        return -1;
    }
    //确保_buf可以容纳可读数据
    assert(need_read <= _ibuf.capacity - _ibuf.length);

    int ret;
    do{
        ret = read(_sockfd, _ibuf.data + _ibuf.length, need_read);
    }while(ret == -1 && errno == EINTR);

    if(ret == 0){
        //对端关闭
        if(_name != NULL){
            printf("%s client: connection close by peer!\n", _name);
        }else{
            printf("client: connection close by peer!\n");
        }
        this->clean_conn();
        return -1;
    }else if(ret == -1){
        //读取错误
        fprintf(stderr, "client::do_read() error\n");
        this->clean_conn();
        return -1;
    }

    assert(ret == need_read);
    _ibuf.length += ret;

    //2. 解包
    msg_head head;
    int msgid, msglen;
    while(_ibuf.length >= MESSAGE_HEAD_LEN){
        //解析头部
        memcpy(&head, _ibuf.data + _ibuf.head, MESSAGE_HEAD_LEN);
        msgid = head.msgid;
        msglen = head.msglen;

        //判断消息是否完整到达
        if(msglen > _ibuf.length - MESSAGE_HEAD_LEN){
            //消息没有完整到达
            break;
        }

        //头部读取完毕
        _ibuf.pop(MESSAGE_HEAD_LEN);

    //3. 交给业务函数处理
        // if(_msg_callback != nullptr){
        //     this->_msg_callback(_ibuf.data + _ibuf.head, msglen, msgid, this, NULL);
        // }
        this->_router.call(msgid, msglen, _ibuf.data + _ibuf.head, this);
        //数据区域处理完毕
        _ibuf.pop(msglen);

    }

    //重置head指针
    _ibuf.adjust();

    return 0;
}

//处理写业务  输出缓冲区--->socket套接字
int tcp_client::do_write(){
    //数据有长度，切头部索引是起始位置
    assert(_obuf.head == 0 && _obuf.length != 0);

    int ret;

    while(_obuf.length){
        //写数据
        do{
            ret = write(_sockfd, _obuf.data + _obuf.head, _obuf.length);
        }while(ret == -1 && errno == EINTR);

        if(ret > 0){
            //写成功
            _obuf.pop(ret);
            _obuf.adjust();
        }else if(ret == -1 && errno != EAGAIN){
            fprintf(stderr, "tcp client write error\n");
            this->clean_conn();
        }else{
            //写缓冲区已经满了，等待下一次EPOLLOUT事件
            break;
        }
    }

    if(_obuf.length == 0){
        //已经写完，删除写事件
        printf("write complete, del event EPOLLOUT\n");
        this->_loop->del_io_event(_sockfd, EPOLLOUT);
    }

    return 0;
}

//释放链接资源，重置连接
void tcp_client::clean_conn(){
    if(_sockfd != -1){
        printf("clean conn, del socket!\n");
        this->_loop->del_io_event(_sockfd);
        close(_sockfd);
    }

    connected = false;

    //重新连接
    this->do_connect();
}

tcp_client::~tcp_client(){
    close(_sockfd);
}

//用户---->输出缓冲区
int tcp_client::send_message(const char* data, int msglen, int msgid){
    if(connected == false){
        fprintf(stderr, "no connect, send message stop!\n");
        return -1;
    }
    //是否需要添加写事件触发
    //如果obuf中有数据，没必要添加，如果没有数据，添加完数据需要触发
    bool need_add_event = _obuf.length == 0 ? true : false;

    //封装消息头
    msg_head head;
    head.msgid = msgid;
    head.msglen = msglen;

    memcpy(_obuf.data + _obuf.head + _obuf.length, &head, MESSAGE_HEAD_LEN);
    _obuf.length += MESSAGE_HEAD_LEN;

    memcpy(_obuf.data + _obuf.head + _obuf.length, data, msglen);
    _obuf.length += msglen;

    if(need_add_event){
        //添加写事件
        _loop->add_io_event(_sockfd, write_callback, EPOLLOUT, this);
    }   

    return 0;
}