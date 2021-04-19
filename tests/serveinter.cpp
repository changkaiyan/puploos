//中断服务程序入口，分类型处理中断
#include "cpu.h"
#include "asm.h"
#include "table.hpp"
#include "print.hpp"
extern "C"
{
    void serveinter();
}
static void inter_sche()
{
    //先查看是否退出，若退出再置sleep为true
    if (exit_running[running] == true)
    {
        System::systable._interrupt_exitProcess(running, 0);
        exit_running[running] = false;
    }
    //重新调度
    System::systable.schedule();
    //清空TIP位和mtime
    System::objtable.getObject(4).clk.recvData(System::ClockReg::mtime, 0);
    unsigned mip = read_csr(mip);
    mip &= ~MTIP(1);
    mip &= ~MSIP(1);
    write_csr(mip, mip);
}
void serveinter() //注意清除中断挂起位
{
    //调度进程并设置下一个running线程
    unsigned mcause = read_csr(mcause);
    unsigned mip = read_csr(mip);
    //软中断和其他中断并发当次处理,软中断必须同步处理
    if (mcause == MACHINE_SOFTWARE_INTERRUPT or (mip & MSIP(1)) != 0) //软中断号读取寄存器csr[]获得
    {
        //retvalue出现后必须要在保护区内读取，否则会被其他中断破坏数据

        if (process[running].x[10] == 1) //创建进程
        {
            retvalue[running].unsign = System::systable.createProcess(reinterpret_cast<char *>(process[running].x[11]), reinterpret_cast<void *(*)(void *)>(process[running].x[12]), reinterpret_cast<void *>(process[running].x[13]));
        }
        else if (process[running].x[10] == 2) //退出进程
        {
            retvalue[running].unsign = System::systable.exitProcess(process[running].x[11]);
            if (retvalue[running].unsign != false)
                inter_sche(); //调度
        }
        else if (process[running].x[10] == 3) //挂起进程
        {
            retvalue[running].unsign = System::systable.sleepProcess(process[running].x[11]);
            if (retvalue[running].unsign != false)
                inter_sche(); //调度
        }
        else if (process[running].x[10] == 4) //唤醒进程
        {
            retvalue[running].unsign = System::systable.resumeProcess(process[running].x[11]);
        }
        else if (process[running].x[10] == 5) //分配内存
        {
            retvalue[running].void_p = System::systable.memory.kernelMalloc(process[running].x[11]);
        }
        else if (process[running].x[10] == 6) //回收内存
        {
            retvalue[running].unsign = System::systable.memory.kernelFree(reinterpret_cast<void *>(process[running].x[11]));
        }

        unsigned mip = read_csr(mip);
        mip &= ~MSIP(1);
        write_csr(mip, mip);
    }
    if (mcause == MACHINE_EXTERNAL_INTERRUPT) //外部中断
    {
        System::objtable.getObject(1).key.eventOccur();
        unsigned mip = read_csr(mip);
        mip &= ~MEIP(1);
        write_csr(mip, mip);
    }

    if (mcause == MACHINE_TIMER_INTERRUPT) //时钟中断
    {
        //计算sleep状态进程的时间并处理
        System::systable.sleepSubTick();
        //减小running进程的时间
        System::systable.processSubTick(running);
        inter_sche(); //调度
    }
    if (mcause == ILLEGAL_INSTRUCTION) //指令异常
    {
        //退出进程并重新调度
        System::systable.exitProcess(1);
        unsigned mip = read_csr(mip);
        mip &= ~MEXCEPTIONIP(1);
        write_csr(mip, mip);
    }
    if (mcause == LOAD_ACCESS_FAULT) //缺页异常
    {
        unsigned mtval = read_csr(mtval);
        //检查rsw位是否为1，若为0则访存错误，终止进程，否则是请求调页
        if (System::systable.memory.page_is_exist(mtval) == true) //请求分页
        {
            System::systable.memory.swap_in_spec(get_ppn(mtval));
        }
        else
        { //访存错误（段错误）。内核进程默认不会访存错误
            System::printf(0, "Segment Fault!\n");
            System::systable.exitProcess(1);
        }
        unsigned mip = read_csr(mip);
        mip &= ~MEXCEPTIONIP(1);
        write_csr(mip, mip);
    }
}