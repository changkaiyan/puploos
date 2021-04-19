#pragma once
#include "asm.h"
extern "C"
{
    struct Context //默认情况下进程不允许改csr寄存器，不保存csr寄存器
    {
        unsigned x[32];
        unsigned mepc;
        unsigned mstatus;
        unsigned mcause;
        unsigned mtval;
    };
    union Run
    {
        unsigned unsign;
        void* void_p;
    };
    
    extern Context process[MAX_PROCESS_NUM];
    extern int running;
    extern bool sleep_running[];  
    extern bool exit_running[]; 
    extern Run retvalue[MAX_PROCESS_NUM];
}