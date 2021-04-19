#include "asm.h"
.text
.globl _start
.align 4
_start:
    //PUPLOOS操作系统，加载到48MB处，加载8MB，56-64MB内存区间用作系统堆栈保留区
    csrci mstatus, (1<<3)  //关中断
    li sp,64*(1<<20) // 64M位置建栈
    jal osInit  // 跳转入C++语言编写的系统初始化程序
    ebreak //查看处理机状态

    
