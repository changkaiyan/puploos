/*本测试文件包括：存储器类测试，进程与设备中断类测试*/
#include "table.hpp"
#include "util.h"
#include "print.hpp"
#define MEMCHECK(real, expect) printf(0, "real: %x expect: %x equ: %d\n", reinterpret_cast<unsigned>(real), expect, reinterpret_cast<unsigned>(real) == expect)
using namespace System;
static void *test_proc1(void *arg);
static void *test_proc2(void *arg);
void test_memory()
{
    //一般性内存分配
    printf(0, "-----------------test memory------------------\n");
    void *addr = kernelMalloc(1000);
    MEMCHECK(addr, 0);
    //一般性内存访问
    auto tmpp = reinterpret_cast<char *>(addr);
    tmpp[0] = 1;
    void *addr2 = kernelMalloc(1000);
    MEMCHECK(addr2, 4 * 1024);
    //具有内存碎片的内存释放
    kernelFree(addr);
    addr = kernelMalloc(4097);
    MEMCHECK(addr, 8 * 1024);
    void *addr3 = kernelMalloc(1024);
    MEMCHECK(addr3, 0);
    kernelFree(addr2);
    kernelFree(addr3);
    kernelFree(addr);
    //重复性内存释放
    printf(0, "equ: %d\n", kernelFree(addr) == false);
    void *addrlist[20 * 1024]; //（48+32）MB
    //48MB大规模内存分配
    for (int i{0}; i < KERNEL_BEGIN_PPN; ++i)
    {
        unsigned adr = (i << 12);
        //为了加快检测速度不采用软中断调用
        addrlist[i] = System::systable.memory.kernelMalloc(1 << 12);
        // 存储器检查通过
        if (reinterpret_cast<unsigned>(addrlist[i]) != adr)
        {
            MEMCHECK(addrlist[i], adr);
        }
        if (i % 123 == 0)
        {
            printf(0, "Check memory:%d%c\n", i / 123, '%');
        }
    }
    //部分规模内存释放
    printf(0, "equ: %d\n", kernelFree(addrlist[12 * 1024 - 1]) == true);
    addrlist[12 * 1024 - 1] = kernelMalloc(4 * 1024);
    MEMCHECK(addrlist[12 * 1024 - 1], 4 * 1024 * (12 * 1024 - 1));
    //缺页对换验证
    *(reinterpret_cast<unsigned *>(addrlist[0])) = 0xfc6750;
    //验证高地址虚拟存储，具有页面对换的大型内存分配
    addrlist[12 * 1024] = kernelMalloc(4 * 4 * 1024);
    MEMCHECK(addrlist[12 * 1024], 0x4001000);
    if (*reinterpret_cast<unsigned *>(addrlist[12 * 1024]) == 0xfc6750)
    {
        printf(0, "swap error!\n");
    }
    //缺页中断与对换机制
    unsigned test = *(reinterpret_cast<unsigned *>(addrlist[0])); //触发缺页中断
    MEMCHECK(test, 0xfc6750);
    //混合存储释放
    kernelFree(addrlist[12 * 1024]);
    void *tmpy = kernelMalloc(4 * 1024);
    MEMCHECK(tmpy, 0x4001000);

    printf(0, "-----------------test memory------------------\n");
}
void *test_event(Event evt)
{
    printf(0, "event occur!");
    return nullptr;
}
int pid,pid2;
void test_process()
{
    pid = createProcess("proc1", test_proc1, nullptr);
    pid2 = createProcess("proc2", test_proc2, nullptr);
    if (pid == -1 or pid2 == -1)
    {
        while (1)
        {
            System::printf(0, "error\n");
        }
    }
    int i = 0;
    System::printf(0,"proc1:%d,proc2:%d\n",pid,pid2);
    while (1)
    {
        i++;
    }
}
void *test_proc1(void *arg)
{
    System::systable.addProcessEvent(test_event);
    int i = 0;
    sleepProcess(-1);
    while (1)
    {
        printf(0, "test_proc1:%d\n", i++);
        
    }
    return nullptr;
}
void *test_proc2(void *arg)
{
    int i = 0;
    while (1)
    {
        if(i==0x10)
        {
            printf(0, "test_proc2:%d\n", i++);
            printf(0,"proc2resume:%d\n",resumeProcess(pid));
        }
        else
        {
            i++;
        }
        
    }
    return nullptr;
}