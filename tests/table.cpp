#include "table.hpp"
#include "cpu.h"
#include "util.h"
#include "print.hpp"
namespace System
{
    ObjectTable objtable;
    SystemTable systable;
    SystemTable &SystemTable::initializeProcess()
    {
        mapToProcess([](Process &proc) { proc.initialize(); });
        //内核进程放在0号进程处
        running = 0;
        process_table[0].setNext(-1);

        process_table[0].toRunning();
        process_table[0].setTick(1);
        sleep_running[0] = false;
        exit_running[0] = false;
        pending_head = -1;
        ready_head = -1;
        //初始化unused进程队列
        empty = 1;
        for (int i{1}; i < MAX_PROCESS_NUM - 1; ++i)
        {

            process_table[i].setNext(i + 1);
        }
        process_table[MAX_PROCESS_NUM - 1].setNext(-1);
        can_schedule = true;
        return *this;
    }
    void SystemTable::schedule()
    {
        if (ready_head == -1) //就绪队列没有进程，继续运行
        {
            process_table[running].toRunning();//继续进行下一个时钟片
            return;
        }

        if ((process_table[running].getTick() <= 0 and can_schedule) or sleep_running[running] == true)
        {
            if (sleep_running[running] == false) //不需要休眠此进程
            {

                process_table[running].toReady(); //进程PCB转换,不转换next变量
                addToReady(running);              //加入就绪队列
               
            }
            else //用于退出进程和休眠进程
            {
                sleep_running[running] = false;
                
            }
             //摘除就绪队列第一个进程到运行指针
                running = ready_head; //指向下一个运行的进程
                ready_head = process_table[ready_head].getNext();

                process_table[running].setNext(-1);
                process_table[running].toRunning(); //进程PCB转换,不转换next变量
        }
    }
    void SystemTable::addToReady(int processid)
    {
        if (ready_head == -1)
        {
            ready_head = processid;
            process_table[processid].setNext(-1);
            return;
        }
        //查找就绪队列中最后一个进程
        int index{ready_head};
        while (process_table[index].getNext() != -1)
        {
            index = process_table[index].getNext();
        }

        //加入就绪队列
        process_table[index].setNext(processid);
        process_table[processid].setNext(-1);
    }
    proc_t SystemTable::createProcess(char *name, void *(*func)(void *), void *arg)
    {
        bool can{false};
        PROTECT_IN(can);
        //进程创建时默认给予1MB存储空间
        void *mem = memory.kernelMalloc(1024 * 1024);
        if (mem == reinterpret_cast<void *>(MEM_FAIL) or empty == -1)
        {
            PROTECT_OUT(can);
            return -1;
        }
        //从空闲进程链上取下一个空闲进程
        proc_t processid = empty;
        empty = process_table[empty].getNext();
        //初始化进程
        process_table[processid].initialize(name, Status::READY, 1, mem, 1024 * 1024);
        sleep_running[processid] = false;
        exit_running[processid] = false;
        
        addToReady(processid);
        //初始化进程上下文
        memset(&process[processid], 0, sizeof(process[processid]));
        process[processid].mepc = reinterpret_cast<unsigned>(func);
        process[processid].x[2] = reinterpret_cast<unsigned>(mem + 1024 * 1024); //新建进程栈
        //初始化进程参数
        process[processid].x[10] = reinterpret_cast<unsigned>(arg);
        PROTECT_OUT(can);
        return processid;
    }
    bool SystemTable::exitProcess(int retvalue)
    {
        exit_running[running] = true;
        return true;
    }
    bool SystemTable::_interrupt_exitProcess(proc_t process_id, int retvalue) //应当在中断中退出进程
    {
        //进程在运行态
        if (process_table[process_id].getStatus() == Status::RUNNING)
        {
            //不能调度
            if (ready_head == -1)
            {
                return false;
            }
            //回收进程资源
            if (memory.kernelFree(process_table[process_id].getBase()) == false)
            {
                return false;
            }
            //改变进程状态
            process_table[process_id].toUnused(retvalue);
            sleep_running[running] = true; //便于剩余进程的调度
        }

        //把进程加入到空队列中
        process_table[process_id].setNext(empty);
        empty = process_id;
        return true;
    }
    void SystemTable::sleepSubTick()
    {
        if (pending_head == -1)
        {
            return;
        }
        int index{pending_head}, pre{-1};
        while (index != -1)
        {
            process_table[index].subTick();
            if (process_table[index].getTick() == 0)
            {
                //转换状态
                process_table[index].toReady();
                //重新初始化睡眠与退出标志
                sleep_running[index] = false;
                exit_running[index] = false;
                //取下挂起队列
                if (pre == -1)
                {
                    pending_head = process_table[index].getNext();
                    pre = -1;
                }
                else
                {
                    process_table[pre].setNext(process_table[index].getNext());
                    pre = index;
                }
                //加入就绪队列
                addToReady(index);
            }
            else
            {
                pre = index;
            }
            index = process_table[index].getNext();
        }
    }
    /*只在软中断中调用*/
    bool SystemTable::sleepProcess(int tick)
    {
        bool can{false};
        PROTECT_IN(can);

        //判断系统中是否仅有一个进程，如果是则不休眠
        if (ready_head == -1)
        {
            PROTECT_OUT(can);
            return false;
        }
        //挂起当前进程
        process_table[running].setNext(pending_head);
        process_table[running].toPending(tick);
        pending_head = running;
        sleep_running[running] = true;
        PROTECT_OUT(can);
        return true;
    }
    void SystemTable::addProcessEvent(void *(*eventFunc)(Event evt))
    {
        bool can{false};
        PROTECT_IN(can);
        process_table[running].addEventProc(eventFunc);
        PROTECT_OUT(can);
    }

    bool SystemTable::resumeProcess(proc_t process_id)
    {
        bool can{false};
        PROTECT_IN(can);
        if (process_table[process_id].getStatus() != Status::PENDING or pending_head == -1)
        {
            PROTECT_OUT(can);
            return false;
        }
        process_table[process_id].toReady();
        //查找挂起队列中的进程
        int index{pending_head}, pre{-1};
        if (process_id == pending_head)
        {
            pending_head = process_table[process_id].getNext();
            addToReady(process_id);
            PROTECT_OUT(can);
            return true;
        }
        while (index != -1)
        {
            if (process_id == index)
            {
                process_table[pre].setNext(process_table[index].getNext());
                addToReady(process_id);
                break;
            }
            pre = index;
            index = process_table[index].getNext();
        }
        PROTECT_OUT(can);
        return false;
    }
} // namespace System