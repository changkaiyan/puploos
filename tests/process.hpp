#pragma once
#include "asm.h"
#include "object.hpp"
#include "event.hpp"
//进程局部管理

namespace System
{
    using proc_t = int;
    enum class Status
    {
        NOTALLOC,
        RUNNING,
        PENDING,
        READY
    }; //进程状态

    class Process
    {
    public:
        /*进程PCB初始化*/
        Process &initialize(char *pro_name, Status stat, int tick, void *memory_bas, unsigned memory_siz);
        /*重载的默认初始化方法*/
        Process &initialize();
        /*进程接到事件的处理动作*/
        Process &recvEvent(Event event);
        /*获取链上下一个进程号*/
        int getNext()
        {
            return next;
        }
        /*设置当前链上下一个程序*/
        void setNext(int thenext)
        {
            next = thenext;
        }
        /*设置总共tick*/
        void setTick(int tick)
        {
            total_tick = res_tick = tick;
        }
        /*时间链减一*/
        bool subTick()
        {
            if (status == Status::RUNNING) //对于运行时进程
            {
                if (res_tick == 0)
                    return false;
                else
                {
                    res_tick--;
                    return true;
                }
            }
            else if (status == Status::PENDING) //对于挂起的进程
            {
                if (sleep_tick == 0)
                    return false;
                else if (sleep_tick > 0)
                {
                    sleep_tick--;
                    return true;
                }
            }
            return true;
        }
        int getTick()
        {
            if (status != Status::PENDING)
                return res_tick;
            else
            {
                return sleep_tick;
            }
        }
        void *getBase()
        {
            return memory_base;
        }
        /*改为就绪态*/
        void toReady()
        {
            status = Status::READY;
            res_tick = total_tick;
        }
        /*从就绪态改为运行态*/
        void toRunning()
        {
            status = Status::RUNNING;
            res_tick=total_tick;
        }

        /*进程自己挂起sleep*/
        void toPending(int tick)
        {
            status = Status::PENDING;
            sleep_tick = tick;
        }
        /*进程退出exit*/
        void toUnused(int ret);
        /*进程添加自定义事件处理函数*/
        void addEventProc(void *(*eventFunc)(Event evt))
        {
            eventProc = eventFunc;
        }

        Status &getStatus()
        {
            return status;
        }

    private:
        Status status;        //进程状态：0未分配，1运行中，2挂起，3就绪
        int retvalue;         //进程上次结束时返回值
        int next;             //进程表中下一个就绪/挂起/未分配进程
        int total_tick;       //总共允许运行的tick数
        int res_tick;         //剩余的tick数
        int sleep_tick;       //挂起状态的剩余时钟数
        void *memory_base;    //进程空间虚拟基地址
        unsigned memory_size; //进程空间大小（内存管理器可以查表得出，这里是作为备用信息）
        char name[36];        //进程名称
        Object obj_list[16];  //进程局部对象句柄列表
        void *(*eventProc)(Event event); //事件处理函数
        //进程号在systemtable中隐式分配，这里不单列
    };
} // namespace System
