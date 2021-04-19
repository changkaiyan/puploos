#include "asm.h"
#include "table.hpp"
#include "object.hpp"
#include "print.hpp"
#include "util.h"
#include "test.h"
int osInit() //系统初始化函数
{
    //如果程序跑飞请把中断关闭
    System::systable.initializePage().initializeMemory(); //初始化内存容量和分页
    System::objtable.initializeObject();                  //初始化对象
    System::systable.initializeProcess();                 //初始化进程表
    System::objtable.initializeTick();                    //初始化时钟
    System::systable.initializeInterrupt();               //初始化中断
    //注意PUPLOOS不能够用对象指针方式调用对象的方法，否则会查找虚函数表，OS里面不设置虚函数表-fno-rtti

    test_memory();//操作系统启动时进行内存代码段检查可以确保编译无误
    // test_process();//测试进程与中断类模块

    return reinterpret_cast<int>(System::systable.memory.kernelMalloc(20));
}
