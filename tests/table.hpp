#pragma once
#include "asm.h"
#include "process.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "semaphore.hpp"
//用于声明中断处理，进程调度，初始化进程等需要与汇编语言交互的函数
extern "C"
{
    extern void _interrrupt(); //中断处理函数声明，不可写作函数指针，注意两者写法的区别

    int osInit();      //操作系统初始化进程
    void serveinter(); //中断服务函数声明
}
// 建立全局系统表及硬件相关信息
namespace System
{

    class SystemTable //全局系统表，全局内存管理，全局进程管理，全局中断管理
    {
    public:
        /*初始化分页*/
        SystemTable &initializePage()
        {
            memory.initializePage();
            memory.initializeDeviceMemory();
            return *this;
        }

        /*初始化中断*/
        SystemTable &initializeInterrupt()
        {
            //关联中断入口函数
            interruptFunction = _interrrupt;
            //设置中断入口地址寄存器MTVEC
            unsigned mtv = read_csr(mtvec);

            mtv = (reinterpret_cast<unsigned>(interruptFunction) & ~(0b11));
            write_csr(mtvec, mtv);

            //开中断
            enable_irq();

            return *this;
        }
        /*初始化内存4KB块链表，以PPN号为单位*/
        SystemTable &initializeMemory()
        {
            memory.initializeMemory();
            return *this;
        }
        /*初始化进程管理*/
        SystemTable &initializeProcess();

        /*table.hpp中的进程管理是全局进程管理，process.hpp是局部进程管理*/
        /*进程的调度仅仅在中断中进行，这里仅仅进行进程的用户层基本操作*/
        /*创建进程，成功则返回线程号，失败返回-1*/
        proc_t createProcess(char *name, void *(*func)(void *), void *arg);
        /*使进程挂起，同时重新调度。内核不得使用sleep语句*/
        bool sleepProcess(int tick);
        /*使进程就绪*/
        bool resumeProcess(proc_t process_id);
        //添加当前进程的事件处理函数
        void addProcessEvent(void *(*eventFunc)(Event evt));
        /*使进程退出,同时重新调度*/
        bool exitProcess(int retvalue);
        /*改变running指针用于进程调度*/
        void schedule();
        //中断中退出进程
        bool _interrupt_exitProcess(proc_t process_id, int retvalue);
        /*进程间通信机制：事件*/
        bool sendEventTo(Event evt, unsigned processid)
        {
            if (processid >= MAX_PROCESS_NUM or process_table[processid].getStatus() == Status::NOTALLOC)
            {
                return false;
            }
            process_table[processid].recvEvent(evt);
        }
        /**
         * @brief 时钟中断时处理正在运行的进程
         * 
         * @param processid 进程号
         */
        void processSubTick(int processid)
        {
            process_table[processid].subTick();
        }
        /**
         * @brief 时钟中断时检查睡眠进程是否到时间唤醒，并做处理
         * 
         */
        void sleepSubTick();
        /*map到进程表上*/
        void mapToProcess(void (*func)(Process &proc))
        {
            for (int i{0}; i < MAX_PROCESS_NUM; ++i)
            {
                func(process_table[i]);
            }
        }
        
        Memory memory{}; //存储器对象
    private:
        void addToReady(int processid);
        System::Process process_table[MAX_PROCESS_NUM]{}; //进程表

        int ready_head{-1};   //就绪队列中的头部进程下标，尾插头出
        int pending_head{-1}; //等待队列中的头部进程下标，尾插头出
        int empty{-1};        //未分配进程头，
        bool can_schedule{true};    //调度器开关

        void (*interruptFunction)(); //中断入口函数地址（汇编实现）
    };                               //计算机系统句柄（单例模式）
    class ObjectTable                //全局对象表，全局设备管理，全局文件管理，全局进程间同步与通信管理
    {
    public:
        ObjectTable &initializeObject()
        {
            //设备对象初始化
            Serial tty;
            Disk disk;
            Keyboard keyboard;
            Clock clk;
            Swap swap;
            tty.initialize();
            disk.initialize();
            keyboard.initialize();
            clk.initialize();
            //文件系统初始化
            swap.initialize(SWAPBEGIN, SWAPEND, 2); //初始化交换分区对象
            //设备对象分配内存号并挂载
            obj_list[0].ttyS = tty;     //标准输出串口设备
            obj_list[1].key = keyboard; //标准输入键盘设备
            obj_list[2].dis = disk;     //块设备
            obj_list[3].swa = swap;     //交换分区
            obj_list[4].clk = clk;      //时钟设备
            objnum = 5;
            return *this;
        }
        Object &initializeTick()
        {
            obj_list[4].clk.recvData(ClockReg::mtimecmp, 0x90000);
            obj_list[4].clk.recvData(ClockReg::mtime, 0);
        }
        Object &getObject(int number)
        {
            return obj_list[number];
        }
        /*添加信号量：输入信号量初始数值，返回对象编号*/
        int addSemaphore(int n)
        {
            obj_list[objnum++].sema.initialize(n);
            return objnum-1;
        }
    private:
        Object obj_list[MAX_GLB_OBJ_NUM];
        int objnum;

    }; //对象表句柄（单例模式）
    extern ObjectTable objtable;
    extern SystemTable systable;
} // namespace System
