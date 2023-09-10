#pragma once
/*
    定义一个buffer存放数据的结构*/
class io_buf{
    public:
        io_buf* next;//下一个io_buf

        int capacity;//缓冲区总大小
        int length;//缓冲区当前数据大小
        int head;//缓冲区头部位置
        char* data;//当前io_buf所保存的数据地址

        io_buf(int size);//构造函数
        void clear();//清空缓冲区
        void adjust();//将已处理的数据移除，未处理的数据前移
        void copy(const io_buf* other);//将other的数据拷贝到当前io_buf
        void pop(int len);//处理长度为len的数据，移动head和修正length
};