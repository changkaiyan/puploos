#include "util.h"
#include "process.hpp"
#include "print.hpp"
namespace System
{
    Process &Process::initialize(char *pro_name, Status stat, int tick, void* memory_bas, unsigned memory_siz)
    {
        strcpy(name, pro_name);       //拷贝进程名
        status = stat;                //初始化进程状态
        total_tick = res_tick = tick; //初始化时钟
        memory_base = memory_bas;     //初始化虚拟内存基地址
        memory_size = memory_siz;     //初始化虚拟进程容量
        eventProc = nullptr;
    }
    Process &Process::initialize()
    {
        eventProc = nullptr;
        status = Status::NOTALLOC;
        total_tick = res_tick = 0;
    }
    Process &Process::recvEvent(Event event)
    {
        if (eventProc != nullptr)
            eventProc(event);
        return *this;
    }
    void Process::toUnused(int ret)
    {
        status = Status::NOTALLOC;
        retvalue = ret;
        eventProc = nullptr;
    }
} // namespace System