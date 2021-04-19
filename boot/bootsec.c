#include "asm.h"
void (*kernel)() = 0x3000000; //48MB处加载内核
unsigned *rw = 0xfffff300;
unsigned *ready = 0xfffff500;
unsigned *lbaaddr = 0xfffff400;
unsigned *data = 0xfffff600; //128个块，512个字节
void read_data(unsigned *addr, unsigned lbraddr)
{
    *rw = 1;
    *lbaaddr = lbraddr;
    while (*ready == 0)
        ;
    for (int i = 0; i < 128; ++i)
    {
        *(addr + i) = *(data + i);
    }
}
void read_kernel()
{
    //载入8MB内核，共0x4000块,地址偏移8块
    for (int i = 0; i < 0x4000; ++i)
    {
        read_data((i << 9) + kernel, i + 8);
    }
}

void bootloader()
{
    read_kernel();
    kernel();
}
