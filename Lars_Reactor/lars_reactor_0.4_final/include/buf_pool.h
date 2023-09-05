#pragma once

//#include<ext/hash_map>
#include<unordered_map>
#include "io_buf.h"
#include<pthread.h>
#include<cstdint>
#include<cstdio>
#include<cstdlib>
//typedef __gnu_cxx::hash_map<int, io_buf*> pool_t;
typedef std::unordered_map<int, io_buf*> pool_t;

enum MEM_CAP{
    m4K = 4096,
    m16K = 16384,
    m64K = 65536,
    m256K = 262144,
    m1M = 1048576,
    m4M = 4194304,
    m8M = 8388608
};

//总内存池最大限制 5GB
#define EXTRA_MEM_LIMIT (5U * 1024 * 1024)

/*定义内存池 
*设计为单例*/
class buf_pool{
    private:
        buf_pool();
        //拷贝构造私有化
        buf_pool(const buf_pool&);
        //赋值构造私有化
        const buf_pool& operator=(const buf_pool&);

        pool_t _pool;//内存池map集合句柄
        uint64_t _total_mem;//内存池总内存大小
        static buf_pool* _instance;//内存池单例句柄
            
        //用于保证创建单例的init方法只执行一次的锁
        static pthread_once_t _once;
        //用户保护内存池链表修改的互斥锁
        static pthread_mutex_t _mutex;

    public:
        static void init(){//初始化单例对象
            _instance = new buf_pool();
        }
        static buf_pool* instance(){//获取单例对象
            pthread_once(&_once, init);
            return _instance;
        }
        //开辟一个io_buf,默认开辟4K
        io_buf* alloc_buf(int n = m4K);
        
        //重置一个io_buf
        void revert(io_buf* buffer);
};